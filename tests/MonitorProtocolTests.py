"""Run against MonitorServerTestHost.exe, without opening an OpenGL game window."""
import http.client
import json
import pathlib
import socket
import subprocess
import sys
import time


def request(path, method="GET"):
    connection = http.client.HTTPConnection("127.0.0.1", 8765, timeout=3)
    try:
        connection.request(method, path)
        response = connection.getresponse()
        body = response.read()
        assert int(response.getheader("Content-Length")) == len(body)
        assert response.getheader("Cache-Control") == "no-store"
        return response.status, response.getheader("Content-Type"), body
    finally:
        connection.close()


def command(host, text):
    host.stdin.write(text + "\n")
    host.stdin.flush()
    assert host.stdout.readline().strip() == "UPDATED"


def main():
    root = pathlib.Path(__file__).resolve().parents[1]
    host = subprocess.Popen([sys.argv[1]], cwd=root, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, encoding="utf-8")
    try:
        while True:
            line = host.stdout.readline()
            if line.strip() == "READY":
                break
            if not line:
                raise AssertionError("Test host failed to start; close any game using port 8765.")
        for route, content_type in [("/", "text/html"), ("/monitor.css", "text/css"),
                                    ("/monitor.js", "application/javascript")]:
            status, actual_type, body = request(route)
            assert status == 200 and actual_type.startswith(content_type) and body
        status, content_type, body = request("/api/state?test=1")
        assert status == 200 and content_type.startswith("application/json")
        state = json.loads(body)
        assert state["counts"] == {"npcs": 2, "villages": 1, "mobs": 4,
                                   "aliveMobs": 3, "deadMobs": 1}
        assert state["npcs"][0]["name"] == '주민 "가"\\길\n\t🌿'
        assert state["npcs"][0]["position"] == {"x": 12.25, "y": 3.5}
        village = state["villages"][0]
        assert village["mood"] is None and village["rumors"] is None
        assert village["disposition"] is None and village["lakeChoice"] is None
        assert state["hunting"]["types"][1]["dead"] == 1
        assert request("/../SimpleGame.cpp")[0] == 404
        assert request("/api/state", "POST")[0] == 405
        command(host, "pause")
        state = json.loads(request("/api/state")[2])
        assert state["paused"] and all(npc["state"] == "paused" for npc in state["npcs"])
        command(host, "clear")
        assert request("/api/state")[0] == 503
        command(host, "publish")
        assert request("/api/state")[0] == 200
        with socket.create_connection(("127.0.0.1", 8765), timeout=3) as partial:
            partial.sendall(b"GET /api/")
            time.sleep(.05)
            partial.sendall(b"state HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n")
            response = http.client.HTTPResponse(partial)
            response.begin()
            assert response.status == 200
            assert json.loads(response.read())["counts"]["npcs"] == 2
        with socket.create_connection(("127.0.0.1", 8765), timeout=3) as oversized:
            oversized.sendall(b"GET /api/state HTTP/1.1\r\nHost: localhost\r\nX-Test: "
                              + b"x" * 9000 + b"\r\n\r\n")
            try:
                assert not oversized.recv(1024)
            except ConnectionResetError:
                pass
        with socket.create_connection(("127.0.0.1", 8765), timeout=3):
            start = time.monotonic()
            assert request("/api/state")[0] == 200
            assert time.monotonic() - start < 2.5
        with socket.create_connection(("127.0.0.1", 8765), timeout=3):
            host.stdin.write("quit\n")
            host.stdin.flush()
            host.wait(timeout=2)
        assert host.returncode == 0
        print("PASS: JSON/UTF-8/locale, asset routes, counts, pause, 503 recovery, partial requests, oversized headers, idle-client timeout, shutdown.")
    finally:
        if host.poll() is None:
            host.kill()
            host.wait()


if __name__ == "__main__":
    main()
