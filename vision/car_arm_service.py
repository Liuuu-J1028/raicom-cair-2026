"""
小车机械臂抓取 - PC端统一服务
接收ESP32-CAM照片 → YOLO识别齿轮/螺栓 → 计算抓取坐标 → 发送命令给STM32

用法:
  python car_arm_service.py --cam-port COM3 --stm32-port COM4
  
ESP32-CAM通过WiFi发送照片到: http://PC的IP:8080/detect
STM32通过USB串口连接PC
"""
from ultralytics import YOLO
from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import io
import cv2
import numpy as np
import socket
import serial
import serial.tools.list_ports
import time
import threading

class STM32Controller:
    """STM32串口通信控制"""
    
    def __init__(self, port=None, baud_rate=115200):
        self.serial = None
        self.port = port
        self.baud_rate = baud_rate
    
    def find_stm32_port(self):
        """自动查找STM32串口"""
        ports = serial.tools.list_ports.comports()
        for port in ports:
            desc = port.description.upper()
            if 'STM32' in desc or 'STLINK' in desc or 'USB SERIAL' in desc:
                print(f"找到STM32: {port.device} - {port.description}")
                return port.device
        return None
    
    def connect(self):
        if self.port is None:
            self.port = self.find_stm32_port()
        
        if self.port is None:
            print("未找到STM32，请手动指定 --stm32-port COM端口号")
            return False
        
        try:
            self.serial = serial.Serial(self.port, self.baud_rate, timeout=0.5)
            time.sleep(1)
            print(f"已连接STM32: {self.port}")
            return True
        except Exception as e:
            print(f"STM32连接失败: {e}")
            return False
    
    def send_command(self, cmd_type, params):
        """
        发送控制命令给STM32
        
        命令格式 (JSON):
        {
          "cmd": "grab" | "release" | "move_arm" | "stop",
          "x": 100,           # 物体中心x坐标 (像素)
          "y": 200,           # 物体中心y坐标 (像素)
          "class": "gear",    # 物体类别
          "servo_angle": 90   # 舵机角度
        }
        """
        if self.serial and self.serial.is_open:
            msg = json.dumps({"cmd": cmd_type, **params})
            self.serial.write((msg + '\n').encode('utf-8'))
            print(f"  → STM32命令: {msg}")
            return True
        return False
    
    def close(self):
        if self.serial and self.serial.is_open:
            self.serial.close()


class DetectionHandler(BaseHTTPRequestHandler):
    model = None
    stm32 = None
    conf_threshold = 0.3
    frame_width = 640
    frame_height = 480
    
    def do_POST(self):
        if self.path == '/detect':
            content_length = int(self.headers['Content-Length'])
            image_data = self.rfile.read(content_length)
            
            print(f"\n收到ESP32-CAM照片: {len(image_data)} bytes")
            
            nparr = np.frombuffer(image_data, np.uint8)
            frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            
            if frame is None:
                self._send_error("图片解码失败")
                return
            
            self.frame_height, self.frame_width = frame.shape[:2]
            
            results = self.model(frame, conf=self.conf_threshold)
            
            detections = []
            for result in results:
                for box in result.boxes:
                    x1, y1, x2, y2 = box.xyxy[0].tolist()
                    conf = box.conf[0].item()
                    cls = int(box.cls[0].item())
                    class_name = self.model.names[cls]
                    
                    x_center = (x1 + x2) / 2
                    y_center = (y1 + y2) / 2
                    
                    detections.append({
                        'class': class_name,
                        'confidence': round(conf, 3),
                        'x_center': round(x_center),
                        'y_center': round(y_center),
                        'bbox': [round(x1), round(y1), round(x2), round(y2)]
                    })
                    print(f"  检测到: {class_name}  置信度:{conf:.2f}  位置:({x_center:.0f},{y_center:.0f})")
            
            # 识别完成 → 发送抓取命令给STM32
            if detections and self.stm32:
                # 按置信度排序，取最高
                detections.sort(key=lambda x: x['confidence'], reverse=True)
                target = detections[0]
                
                print(f"\n  目标: {target['class']} (置信度:{target['confidence']})")
                print(f"  坐标: x={target['x_center']}, y={target['y_center']}")
                
                # 发送抓取命令
                self.stm32.send_command('grab', {
                    'class': target['class'],
                    'x': target['x_center'],
                    'y': target['y_center'],
                    'confidence': target['confidence']
                })
            
            if not detections:
                print("  未检测到物体")
            
            response = json.dumps({
                'status': 'ok',
                'count': len(detections),
                'objects': detections
            }, ensure_ascii=False)
            
            self._send_json(response)
    
    def do_GET(self):
        if self.path == '/status':
            self._send_json('{"status": "running"}')
        elif self.path == '/':
            html = f"""
            <h2>小车机械臂抓取系统</h2>
            <p>ESP32-CAM拍照 → PC YOLO识别 → STM32控制机械臂</p>
            <hr>
            <p>YOLO模型: 齿轮/螺栓检测</p>
            <p>置信度阈值: {self.conf_threshold}</p>
            <p>图像分辨率: {self.frame_width}x{self.frame_height}</p>
            """
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(html.encode('utf-8'))
        else:
            self.send_error(404)
    
    def _send_json(self, data):
        self.send_response(200)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(data.encode('utf-8') if isinstance(data, str) else json.dumps(data).encode('utf-8'))
    
    def _send_error(self, msg):
        self.send_response(400)
        self.end_headers()
        self.wfile.write(json.dumps({'status': 'error', 'message': msg}).encode('utf-8'))
    
    def log_message(self, format, *args):
        pass


def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    finally:
        s.close()
    return ip


def list_all_ports():
    """列出所有串口"""
    ports = serial.tools.list_ports.comports()
    print("=== 可用串口 ===")
    for p in ports:
        print(f"  {p.device} - {p.description}")
    if not ports:
        print("  未找到串口")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='小车机械臂抓取系统 - PC控制中心')
    parser.add_argument('--port', type=int, default=8080, help='HTTP服务端口')
    parser.add_argument('--conf', type=float, default=0.3, help='置信度阈值')
    parser.add_argument('--model', type=str, 
                        default='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt',
                        help='YOLO模型路径')
    parser.add_argument('--stm32-port', type=str, default=None, 
                        help='STM32串口号 (如 COM4)')
    parser.add_argument('--stm32-baud', type=int, default=115200,
                        help='STM32波特率')
    parser.add_argument('--list-ports', action='store_true',
                        help='列出所有串口')
    
    args = parser.parse_args()
    
    if args.list_ports:
        list_all_ports()
        return
    
    print("=" * 50)
    print("  小车机械臂抓取系统 - PC控制中心")
    print("=" * 50)
    print()
    
    # 1. 连接STM32
    stm32 = STM32Controller(args.stm32_port, args.stm32_baud)
    if not stm32.connect():
        print("⚠ STM32未连接，机械臂功能不可用")
    
    # 2. 加载YOLO模型
    print("正在加载YOLO模型...")
    DetectionHandler.model = YOLO(args.model)
    DetectionHandler.stm32 = stm32
    DetectionHandler.conf_threshold = args.conf
    print("模型加载完成！")
    print()
    
    # 3. 启动HTTP服务
    ip = get_local_ip()
    print(f"本机IP: {ip}")
    print(f"HTTP端口: {args.port}")
    print()
    print("ESP32-CAM配置:")
    print(f"  发送地址: http://{ip}:{args.port}/detect")
    print()
    print("STM32串口:")
    print(f"  端口: {stm32.port if stm32.serial else '未连接'}")
    print()
    print("等待ESP32-CAM发送照片...")
    print("按 Ctrl+C 退出")
    print("=" * 50)
    
    server = HTTPServer(('0.0.0.0', args.port), DetectionHandler)
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n服务已停止")
        server.shutdown()
        if stm32:
            stm32.close()


if __name__ == '__main__':
    main()
