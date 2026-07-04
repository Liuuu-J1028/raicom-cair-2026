"""
ESP32-CAM HTTP接收端 - 接收照片并执行YOLO检测

使用方法:
1. 查看本机IP: ipconfig
2. 运行服务器: python esp32_cam_server.py
3. ESP32-CAM通过WiFi发送照片到 http://你的IP:8080/detect
"""
from ultralytics import YOLO
from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import io
import cv2
import numpy as np
import socket
import threading
import time

class DetectionHandler(BaseHTTPRequestHandler):
    model = None
    conf_threshold = 0.3
    
    def do_POST(self):
        if self.path == '/detect':
            content_length = int(self.headers['Content-Length'])
            image_data = self.rfile.read(content_length)
            
            print(f"收到图片: {len(image_data)} bytes")
            
            # JPEG解码
            nparr = np.frombuffer(image_data, np.uint8)
            frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            
            if frame is None:
                self.send_error(400, "图片解码失败")
                return
            
            # YOLO检测
            results = self.model(frame, conf=self.conf_threshold)
            
            detections = []
            for result in results:
                for box in result.boxes:
                    x1, y1, x2, y2 = box.xyxy[0].tolist()
                    conf = box.conf[0].item()
                    cls = int(box.cls[0].item())
                    class_name = self.model.names[cls]
                    
                    detections.append({
                        'class': class_name,
                        'confidence': round(conf, 3)
                    })
                    print(f"  检测到: {class_name}, 置信度: {conf:.2f}")
            
            if not detections:
                print("  未检测到物体")
            
            response = json.dumps({
                'status': 'ok',
                'count': len(detections),
                'objects': detections
            }, ensure_ascii=False)
            
            self.send_response(200)
            self.send_header('Content-Type', 'application/json; charset=utf-8')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(response.encode('utf-8'))
        
        elif self.path == '/status':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status": "running"}')
        
        else:
            self.send_error(404)
    
    def do_GET(self):
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status": "running"}')
        elif self.path == '/':
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(b'<h1>ESP32-CAM YOLO Detection Server</h1><p>Running...</p>')
        else:
            self.send_error(404)
    
    def log_message(self, format, *args):
        # 简化日志输出
        pass

def get_local_ip():
    """获取本机IP地址"""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    finally:
        s.close()
    return ip

def run_server(model_path='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt', 
               port=8080, conf=0.3):
    print("=== ESP32-CAM YOLO检测服务 ===")
    print()
    print("正在加载YOLO模型...")
    DetectionHandler.model = YOLO(model_path)
    DetectionHandler.conf_threshold = conf
    print("模型加载完成！")
    print()
    
    ip = get_local_ip()
    print(f"本机IP: {ip}")
    print(f"服务端口: {port}")
    print()
    print("ESP32-CAM配置信息:")
    print(f"  PC端接收地址: http://{ip}:{port}/detect")
    print()
    print("等待ESP32-CAM发送照片...")
    print("按 Ctrl+C 退出")
    print("=" * 40)
    
    server = HTTPServer(('0.0.0.0', port), DetectionHandler)
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n服务已停止")
        server.shutdown()

if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='ESP32-CAM YOLO检测服务')
    parser.add_argument('--port', type=int, default=8080, help='服务端口')
    parser.add_argument('--conf', type=float, default=0.3, help='置信度阈值')
    parser.add_argument('--model', type=str, 
                        default='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt',
                        help='模型路径')
    
    args = parser.parse_args()
    run_server(args.model, args.port, args.conf)
