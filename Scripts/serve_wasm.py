"""Serve the packaged Wasm build locally with cross-origin isolation headers.

Usage:  python Scripts/serve_wasm.py [build_dir] [port]
        defaults: Build/Wasm, 8080  ->  open http://localhost:8080/

Sends COOP/COEP/CORP (needed for SharedArrayBuffer / wasm threads), serves the
precompressed .br files when the browser accepts Brotli, and disables caching.
"""
import http.server
import mimetypes
import os
import sys
from functools import partial

ROOT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else
                       os.path.join(os.path.dirname(__file__), "..", "Build", "Wasm"))
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

mimetypes.add_type("application/wasm", ".wasm")
mimetypes.add_type("text/javascript", ".js")
mimetypes.add_type("application/json", ".json")


class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        self.send_header("Cache-Control", "no-cache")
        super().end_headers()

    def send_head(self):
        path = self.translate_path(self.path)
        if os.path.isdir(path):
            path = os.path.join(path, "index.html")
        br = path + ".br"
        if (not path.endswith(".br") and os.path.isfile(br)
                and "br" in self.headers.get("Accept-Encoding", "")):
            f = open(br, "rb")
            self.send_response(200)
            self.send_header("Content-Type", self.guess_type(path))
            self.send_header("Content-Encoding", "br")
            self.send_header("Content-Length", str(os.fstat(f.fileno()).st_size))
            self.send_header("Vary", "Accept-Encoding")
            self.end_headers()
            return f
        return super().send_head()


if __name__ == "__main__":
    os.chdir(ROOT)
    print(f"Serving {ROOT}\nOpen http://localhost:{PORT}/  (Ctrl+C to stop)")
    http.server.ThreadingHTTPServer(("localhost", PORT), partial(Handler, directory=ROOT)).serve_forever()
