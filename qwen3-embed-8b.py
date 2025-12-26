import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from sentence_transformers import SentenceTransformer

model = SentenceTransformer("Qwen/Qwen3-Embedding-8B").cuda()

def generate_embedding(text):
    text = str(text)

    query_embeddings = model.encode([text])
    print(query_embeddings.shape)
    query_embeddings.reshape(4096)
    r = query_embeddings[0].tolist()
    print(r)
    return(r)

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):

    def do_POST(self):
        if self.path == "/embedding":

            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            data = json.loads(post_data.decode('utf-8'))

            text = data.get('content', '')
            print(text)
            
            result = generate_embedding(text)
            print(result)
            response = '[{"index":0,"embedding":['+str(result)+']}]'

            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()

            self.wfile.write(response.encode('utf-8'))

        else:
            self.send_response(404)
            self.end_headers()


def run(server_class=HTTPServer,
        hander_class=SimpleHTTPRequestHandler, port=8081):

    server_address = ('0.0.0.0',port)
    httpd = server_class(server_address, hander_class)
    print("Starting HTTP server on port " + str(port))

    httpd.serve_forever()

if __name__ == '__main__':
    run()
