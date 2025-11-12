#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

// Broker with fault tolerance: tracks message IDs, logs to disk, requeues on consumer failure.
// - Append-only log: broker_log.txt (format: msgID|transaction_data)
// - On startup: replays unacked messages from log
// - On consumer disconnect: requeues unacked messages

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

struct Message {
    uint64_t id;
    std::string data;
    bool acked;
};

static std::ofstream log_file;
static uint64_t next_msg_id = 1;

static void log_message(uint64_t id, const std::string& data) {
    if (log_file.is_open()) {
        log_file << id << "|" << data << std::endl;
        log_file.flush();
    }
}

static std::map<uint64_t, Message> load_log() {
    std::map<uint64_t, Message> msgs;
    std::ifstream infile("broker_log.txt");
    if (!infile.is_open()) return msgs;
    std::string line;
    while (std::getline(infile, line)) {
        size_t pos = line.find('|');
        if (pos == std::string::npos) continue;
        uint64_t id = std::stoull(line.substr(0, pos));
        std::string data = line.substr(pos + 1);
        msgs[id] = {id, data, false};
        if (id >= next_msg_id) next_msg_id = id + 1;
    }
    infile.close();
    std::cout << "Loaded " << msgs.size() << " unacked messages from log" << std::endl;
    return msgs;
}

int main(int argc, char* argv[]) {
    uint16_t producer_port = 9100;
    uint16_t consumer_port = 9200;
    if (argc >= 3) {
        producer_port = static_cast<uint16_t>(std::stoi(argv[1]));
        consumer_port = static_cast<uint16_t>(std::stoi(argv[2]));
    }

    std::cout << "=== Fault-Tolerant Broker ===" << std::endl;
    std::cout << "Producer port: " << producer_port << ", Consumer port: " << consumer_port << std::endl;

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
    if (prod_listen < 0 || cons_listen < 0) return 1;

    // State
    std::set<int> producers;           // connected producer sockets
    std::vector<int> consumers;        // connected consumer sockets
    std::map<int, uint64_t> pending;   // consumer fd -> pending message ID
    size_t rr_index = 0;               // round-robin index
    std::map<int, std::string> inbuf;  // input buffers per socket

    // Main loop using select()
    while (true) {
        fd_set rfds; FD_ZERO(&rfds);
        int maxfd = 0;
        FD_SET(prod_listen, &rfds); maxfd = std::max(maxfd, prod_listen);
        FD_SET(cons_listen, &rfds); maxfd = std::max(maxfd, cons_listen);
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
                consumers.push_back(fd);
                inbuf[fd] = std::string();
                std::cout << "Consumer connected: " << inet_ntoa(cli.sin_addr) << std::endl;
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
                // Send ACK to producer for receipt
                const char* ack = "ACK\n";
                send(p, ack, strlen(ack), 0);
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
                    if (pending.count(c)) {
                        uint64_t msg_id = pending[c];
                        messages[msg_id].acked = true;
                        pending.erase(c);
                    }
                }
            }
        }
        for (int fd : to_close) {
            std::cout << "Consumer disconnected";
            // Requeue unacked message if any
            if (pending.count(fd)) {
                uint64_t msg_id = pending[fd];
                queue.push(msg_id);
                pending.erase(fd);
                std::cout << " (requeued message " << msg_id << ")";
            }
            std::cout << std::endl;
            close(fd);
            consumers.erase(std::remove(consumers.begin(), consumers.end(), fd), consumers.end());
            inbuf.erase(fd);
            if (rr_index >= consumers.size()) rr_index = 0;
        }

        // Dispatch queued messages to consumers (simple round-robin)
        while (!queue.empty() && !consumers.empty()) {
            // Find next available consumer (one without pending message)
            size_t checked = 0;
            int c = -1;
            while (checked < consumers.size()) {
                int candidate = consumers[rr_index];
                if (!pending.count(candidate)) {
                    c = candidate;
                    break;
                }
                rr_index = (rr_index + 1) % consumers.size();
                checked++;
            }
            // All consumers busy? Wait for ACKs
            if (c == -1) break;
            
            uint64_t msg_id = queue.front();
            if (!messages.count(msg_id)) { queue.pop(); continue; }
            Message& msg = messages[msg_id];
            if (msg.acked) { queue.pop(); continue; }
            
            std::string line = msg.data;
            line.push_back('\n');
            ssize_t n = send(c, line.c_str(), line.size(), 0);
            if (n <= 0) {
                // consumer likely disconnected; will be handled next loop
                break;
            }
            queue.pop();
            pending[c] = msg_id;
            rr_index = (rr_index + 1) % consumers.size();
        }
    }

    // Cleanup
    for (int p : producers) close(p);
    for (int c : consumers) close(c);
    close(prod_listen);
    close(cons_listen);
    log_file.close();
    return 0;
}
