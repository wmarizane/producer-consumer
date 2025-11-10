#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

// Minimal broker: accepts producers on producer_port, consumers on consumer_port.
// Reads newline-delimited messages from producers, enqueues them, and forwards
// to connected consumers in round-robin. No persistence or ACK tracking yet.

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

int main(int argc, char* argv[]) {
    uint16_t producer_port = 9100;
    uint16_t consumer_port = 9200;
    if (argc >= 3) {
        producer_port = static_cast<uint16_t>(std::stoi(argv[1]));
        consumer_port = static_cast<uint16_t>(std::stoi(argv[2]));
    }

    std::cout << "=== Simple Broker ===" << std::endl;
    std::cout << "Producer port: " << producer_port << ", Consumer port: " << consumer_port << std::endl;

    int prod_listen = make_server(producer_port);
    int cons_listen = make_server(consumer_port);
    if (prod_listen < 0 || cons_listen < 0) return 1;

    // State
    std::set<int> producers;           // connected producer sockets
    std::vector<int> consumers;        // connected consumer sockets
    size_t rr_index = 0;               // round-robin index
    std::map<int, std::string> inbuf;  // input buffers per socket
    std::queue<std::string> queue;     // message queue

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
                queue.push(line);
                // Send ACK to producer for receipt
                const char* ack = "ACK\n";
                send(p, ack, strlen(ack), 0);
            }
        }
        for (int fd : to_close) {
            std::cout << "Producer disconnected" << std::endl;
            close(fd); producers.erase(fd); inbuf.erase(fd);
        }

        // Drain consumer input (optional ACKs)
        to_close.clear();
        for (int c : consumers) {
            if (!FD_ISSET(c, &rfds)) continue;
            ssize_t n = recv(c, buf, sizeof(buf), 0);
            if (n <= 0) { to_close.push_back(c); continue; }
            // Ignore content for now; later we can parse ACKs
        }
        for (int fd : to_close) {
            std::cout << "Consumer disconnected" << std::endl;
            close(fd);
            consumers.erase(std::remove(consumers.begin(), consumers.end(), fd), consumers.end());
            inbuf.erase(fd);
            if (rr_index >= consumers.size()) rr_index = 0;
        }

        // Dispatch queued messages to consumers (simple round-robin)
        while (!queue.empty() && !consumers.empty()) {
            int c = consumers[rr_index];
            std::string line = queue.front();
            line.push_back('\n');
            ssize_t n = send(c, line.c_str(), line.size(), 0);
            if (n <= 0) {
                // consumer likely disconnected; will be handled next loop
                break;
            }
            queue.pop();
            rr_index = (rr_index + 1) % consumers.size();
        }
    }

    // Cleanup
    for (int p : producers) close(p);
    for (int c : consumers) close(c);
    close(prod_listen);
    close(cons_listen);
    return 0;
}
