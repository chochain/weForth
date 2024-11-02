#!/usr/bin/env python3

# It's python3 -m http.server PORT for COOP and COEP enabled

import http.server
import socketserver

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy",   "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "credentialless")
        super().end_headers()

if __name__ == "__main__":
    PORT = 8000
    with socketserver.TCPServer(("", PORT), MyHTTPRequestHandler) as httpd:
        print("Serving at port", PORT)
        httpd.serve_forever()

