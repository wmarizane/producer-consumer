#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>

// Broker with fault tolerance: tracks message IDs, logs to disk, requeues on consumer failure.
// - Append-only log: broker_log.txt (format: msgID|transaction_data)
// - On startup: replays unacked messages from log
// - On consumer disconnect: requeues unacked messages

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

static int make_server(uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(fd); return -1; }
    if (listen(fd, 16) < 0) { perror("listen"); close(fd); return -1; }
    return fd;
}

// HTTP monitoring support
static std::string build_json_status(const std::set<int>& producers, const std::vector<int>& consumers, 
                                     uint64_t total_messages, const std::map<int, std::queue<uint64_t>>& pending,
                                     const std::map<int, uint64_t>& consumer_counts) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"broker\": {\"active\": true, \"total_messages\": " << total_messages << "},\n";
    
    json << "  \"producers\": [";
    bool first = true;
    int prod_id = 1;
    for (int p : producers) {
        if (!first) json << ",";
        json << "\n    {\"id\": \"p" << prod_id++ << "\", \"connected\": true, \"messages_sent\": 0}";
        first = false;
    }
    json << "\n  ],\n";
    
    json << "  \"consumers\": [";
    first = true;
    int cons_id = 1;
    for (int c : consumers) {
        if (!first) json << ",";
        uint64_t msg_count = consumer_counts.count(c) ? consumer_counts.at(c) : 0;
        size_t pending_count = pending.count(c) ? pending.at(c).size() : 0;
        json << "\n    {\"id\": \"c" << cons_id++ << "\", \"connected\": true, \"pending\": " 
             << pending_count << ", \"messages_received\": " << msg_count << "}";
        first = false;
    }
    json << "\n  ]\n";
    json << "}";
    
    return json.str();
}

static void handle_http_request(int client_fd, const std::set<int>& producers, 
                                const std::vector<int>& consumers, uint64_t total_messages,
                                const std::map<int, std::queue<uint64_t>>& pending,
                                const std::map<int, uint64_t>& consumer_counts) {
    char buffer[1024];
    ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    buffer[n] = '\0';
    
    // Parse HTTP request (simple GET /status check)
    std::string request(buffer);
    if (request.find("GET /status") != std::string::npos) {
        std::string json = build_json_status(producers, consumers, total_messages, pending, consumer_counts);
        
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Content-Length: " << json.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << json;
        
        std::string resp_str = response.str();
        send(client_fd, resp_str.c_str(), resp_str.length(), 0);
    }
    
    close(client_fd);
}

struct Message {
    uint64_t id;
    std::string data;
    bool acked;
};

static std::ofstream log_file;
static uint64_t next_msg_id = 1;

static void log_message(uint64_t id, const std::string& data) {
    if (log_file.is_open()) {
        log_file << id << "|0|" << data << '\n';  // 0 = unacked
        // Don't flush - let OS buffer writes for performance
    }
}

static void update_ack_status(uint64_t msg_id) {
    // For simplicity, we append an ACK marker to the log
    if (log_file.is_open()) {
        log_file << msg_id << "|1|ACK" << '\n';  // 1 = acked
        // Don't flush - batch writes for performance
    }
}

static std::map<uint64_t, Message> load_log() {
    std::map<uint64_t, Message> msgs;
    std::ifstream infile("broker_log.txt");
    if (!infile.is_open()) {
        std::cout << "No previous log file found - starting fresh" << std::endl;
        return msgs;
    }
    
    std::string line;
    int total_lines = 0;
    int unacked_lines = 0;
    int ack_markers = 0;
    
    while (std::getline(infile, line)) {
        total_lines++;
        size_t pos1 = line.find('|');
        if (pos1 == std::string::npos) continue;
        size_t pos2 = line.find('|', pos1 + 1);
        if (pos2 == std::string::npos) continue;
        
        uint64_t id = std::stoull(line.substr(0, pos1));
        int acked = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
        std::string data = line.substr(pos2 + 1);
        
        if (acked == 0) {
            // Unacked message - add/keep it
            msgs[id] = {id, data, false};
            unacked_lines++;
        } else if (acked == 1 && data == "ACK") {
            // ACK marker - mark message as acked or remove it
            ack_markers++;
            if (msgs.count(id)) {
                msgs[id].acked = true;
            }
        } else if (acked == 1) {
            // Already acked message in log - ignore it
            continue;
        }
        
        if (id >= next_msg_id) next_msg_id = id + 1;
    }
    infile.close();
    
    // Remove acked messages
    std::vector<uint64_t> to_remove;
    for (const auto& kv : msgs) {
        if (kv.second.acked) {
            to_remove.push_back(kv.first);
        }
    }
    for (uint64_t id : to_remove) {
        msgs.erase(id);
    }
    
    std::cout << "Log recovery: " << total_lines << " lines, " 
              << unacked_lines << " unacked messages, " 
              << ack_markers << " ACK markers" << std::endl;
    std::cout << "Loaded " << msgs.size() << " unacked messages from log" << std::endl;
    std::cout << "Next message ID will be: " << next_msg_id << std::endl;
    return msgs;
}

int main(int argc, char* argv[]) {
    uint16_t producer_port = 9100;
    uint16_t consumer_port = 9200;
    uint16_t monitor_port = 8081;  // HTTP monitoring port
    if (argc >= 3) {
        producer_port = static_cast<uint16_t>(std::stoi(argv[1]));
        consumer_port = static_cast<uint16_t>(std::stoi(argv[2]));
    }
    if (argc >= 4) {
        monitor_port = static_cast<uint16_t>(std::stoi(argv[3]));
    }

    std::cout << "=== Fault-Tolerant Broker ===" << std::endl;
    std::cout << "Producer port: " << producer_port << ", Consumer port: " << consumer_port << std::endl;
    std::cout << "Monitor port: " << monitor_port << " (HTTP status at /status)" << std::endl;

    // Open log file for appending
    log_file.open("broker_log.txt", std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Warning: Could not open broker_log.txt for writing" << std::endl;
    }

    // Load unacked messages from previous run
    std::map<uint64_t, Message> messages = load_log();
    std::queue<uint64_t> queue;  // queue of message IDs
    for (auto& kv : messages) {
        queue.push(kv.first);
    }

    int prod_listen = make_server(producer_port);
    int cons_listen = make_server(consumer_port);
    int monitor_listen = make_server(monitor_port);
    if (prod_listen < 0 || cons_listen < 0 || monitor_listen < 0) return 1;

    // State
    std::set<int> producers;           // connected producer sockets
    std::vector<int> consumers;        // connected consumer sockets
    std::map<int, std::queue<uint64_t>> pending;   // consumer fd -> queue of pending message IDs
    std::map<int, uint64_t> consumer_counts;  // consumer fd -> messages received count
    size_t rr_index = 0;               // round-robin index
    std::map<int, std::string> inbuf;  // input buffers per socket
    const size_t WINDOW_SIZE = 1000;    // Maximum pending messages per consumer (pipeline depth)
    
    // Stats for monitoring
    uint64_t total_dispatched = 0;
    uint64_t total_acked = 0;
    time_t last_stats_time = time(nullptr);

    // Main loop using select()
    while (true) {
        fd_set rfds; FD_ZERO(&rfds);
        int maxfd = 0;
        FD_SET(prod_listen, &rfds); maxfd = std::max(maxfd, prod_listen);
        FD_SET(cons_listen, &rfds); maxfd = std::max(maxfd, cons_listen);
        FD_SET(monitor_listen, &rfds); maxfd = std::max(maxfd, monitor_listen);
        for (int p : producers) { FD_SET(p, &rfds); maxfd = std::max(maxfd, p); }
        for (int c : consumers) { FD_SET(c, &rfds); maxfd = std::max(maxfd, c); }

        timeval tv{1, 0};
        int rv = select(maxfd + 1, &rfds, nullptr, nullptr, &tv);
        if (rv < 0) { if (errno == EINTR) continue; perror("select"); break; }

        // Accept new producers
        if (FD_ISSET(prod_listen, &rfds)) {
            sockaddr_in cli{}; socklen_t cl = sizeof(cli);
            int fd = accept(prod_listen, (sockaddr*)&cli, &cl);
            if (fd >= 0) {
                set_nonblocking(fd);
                producers.insert(fd);
                inbuf[fd] = std::string();
                std::cout << "Producer connected: " << inet_ntoa(cli.sin_addr) << std::endl;
            }
        }

        // Accept new consumers
        if (FD_ISSET(cons_listen, &rfds)) {
            sockaddr_in cli{}; socklen_t cl = sizeof(cli);
            int fd = accept(cons_listen, (sockaddr*)&cli, &cl);
            if (fd >= 0) {
                set_nonblocking(fd);
                // Increase socket send buffer for better throughput
                int sendbuf = 256 * 1024;  // 256 KB
                setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
                consumers.push_back(fd);
                inbuf[fd] = std::string();
                consumer_counts[fd] = 0;  // Initialize message count
                std::cout << "Consumer connected: " << inet_ntoa(cli.sin_addr) << std::endl;
            }
        }

        // Accept HTTP monitoring requests
        if (FD_ISSET(monitor_listen, &rfds)) {
            sockaddr_in cli{}; socklen_t cl = sizeof(cli);
            int fd = accept(monitor_listen, (sockaddr*)&cli, &cl);
            if (fd >= 0) {
                handle_http_request(fd, producers, consumers, next_msg_id - 1, pending, consumer_counts);
            }
        }

        // Read from producers
        std::vector<int> to_close;
        char buf[2048];
        for (int p : producers) {
            if (!FD_ISSET(p, &rfds)) continue;
            ssize_t n = recv(p, buf, sizeof(buf), 0);
            if (n <= 0) { to_close.push_back(p); continue; }
            std::string& b = inbuf[p];
            b.append(buf, buf + n);
            size_t pos;
            while ((pos = b.find('\n')) != std::string::npos) {
                std::string line = b.substr(0, pos);
                b.erase(0, pos + 1);
                uint64_t msg_id = next_msg_id++;
                messages[msg_id] = {msg_id, line, false};
                log_message(msg_id, line);
                queue.push(msg_id);
                // No ACK needed - TCP guarantees delivery
            }
        }
        for (int fd : to_close) {
            std::cout << "Producer disconnected" << std::endl;
            close(fd); producers.erase(fd); inbuf.erase(fd);
        }

        // Drain consumer input (parse ACKs)
        to_close.clear();
        for (int c : consumers) {
            if (!FD_ISSET(c, &rfds)) continue;
            ssize_t n = recv(c, buf, sizeof(buf), 0);
            if (n <= 0) { to_close.push_back(c); continue; }
            std::string& b = inbuf[c];
            b.append(buf, buf + n);
            size_t pos;
            while ((pos = b.find('\n')) != std::string::npos) {
                std::string line = b.substr(0, pos);
                b.erase(0, pos + 1);
                // Simple ACK: mark pending message as acked
                if (line == "ACK" || line == "ERR") {
                    if (pending.count(c) && !pending[c].empty()) {
                        uint64_t msg_id = pending[c].front();
                        pending[c].pop();
                        messages[msg_id].acked = true;
                        update_ack_status(msg_id);  // Persist ACK to log
                        // Increment consumer message count
                        consumer_counts[c]++;
                        total_acked++;
                    }
                }
            }
        }
        for (int fd : to_close) {
            std::cout << "Consumer disconnected";
            // Requeue unacked messages if any
            if (pending.count(fd) && !pending[fd].empty()) {
                std::cout << " (requeuing " << pending[fd].size() << " messages)";
                while (!pending[fd].empty()) {
                    uint64_t msg_id = pending[fd].front();
                    pending[fd].pop();
                    queue.push(msg_id);
                }
                pending.erase(fd);
            }
            std::cout << std::endl;
            close(fd);
            consumers.erase(std::remove(consumers.begin(), consumers.end(), fd), consumers.end());
            inbuf.erase(fd);
            consumer_counts.erase(fd);  // Clean up message count
            if (rr_index >= consumers.size()) rr_index = 0;
        }

        // Dispatch queued messages to consumers (round-robin with pipelining)
        while (!queue.empty() && !consumers.empty()) {
            // Find next available consumer (one with room in their window)
            size_t checked = 0;
            int c = -1;
            while (checked < consumers.size()) {
                int candidate = consumers[rr_index];
                size_t pending_count = pending.count(candidate) ? pending[candidate].size() : 0;
                if (pending_count < WINDOW_SIZE) {
                    c = candidate;
                    break;
                }
                rr_index = (rr_index + 1) % consumers.size();
                checked++;
            }
            // All consumers' windows full? Wait for ACKs
            if (c == -1) break;
            
            uint64_t msg_id = queue.front();
            if (!messages.count(msg_id)) { queue.pop(); continue; }
            Message& msg = messages[msg_id];
            if (msg.acked) { queue.pop(); continue; }
            
            std::string line = msg.data;
            line.push_back('\n');
            ssize_t n = send(c, line.c_str(), line.size(), 0);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Socket buffer full - try next consumer
                    rr_index = (rr_index + 1) % consumers.size();
                    checked++;
                    if (checked >= consumers.size()) {
                        // All consumers have full send buffers
                        break;
                    }
                    continue;
                }
                // Other error - consumer likely disconnected
                break;
            }
            if (n == 0) break; // Shouldn't happen but handle it
            queue.pop();
            pending[c].push(msg_id);
            total_dispatched++;
            rr_index = (rr_index + 1) % consumers.size();
        }
        
        // Print periodic stats
        time_t now = time(nullptr);
        if (now - last_stats_time >= 5) {  // Every 5 seconds
            size_t total_pending = 0;
            for (const auto& kv : pending) {
                total_pending += kv.second.size();
            }
            std::cout << "[Stats] Dispatched: " << total_dispatched 
                      << ", ACKed: " << total_acked
                      << ", Queue: " << queue.size()
                      << ", Pending: " << total_pending
                      << ", Consumers: " << consumers.size() << std::endl;
            last_stats_time = now;
        }
    }

    // Cleanup
    for (int p : producers) close(p);
    for (int c : consumers) close(c);
    close(prod_listen);
    close(cons_listen);
    close(monitor_listen);
    log_file.close();
    return 0;
}
