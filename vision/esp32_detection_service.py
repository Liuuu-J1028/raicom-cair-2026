"""
ESP32 串口通信 + YOLO 图像检测服务
ESP32通过串口发送指令，PC接收指令后进行图像检测，返回结果
"""
from ultralytics import YOLO
import cv2
import serial
import serial.tools.list_ports
import time
import json
import threading

class ESP32DetectionService:
    def __init__(self, model_path='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt', 
                 serial_port=None, baud_rate=115200, camera_index=0, conf_threshold=0.3):
        """
        初始化检测服务
        
        Args:
            model_path: YOLO模型路径
            serial_port: ESP32串口号 (如 COM3, /dev/ttyUSB0)
            baud_rate: 波特率
            camera_index: 摄像头索引
            conf_threshold: 置信度阈值
        """
        print("正在加载YOLO模型...")
        self.model = YOLO(model_path)
        print("模型加载完成！")
        
        self.camera_index = camera_index
        self.conf_threshold = conf_threshold
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.serial = None
        self.cap = None
        self.running = False
        
    def find_esp32_port(self):
        """自动查找ESP32串口"""
        ports = serial.tools.list_ports.comports()
        print("\n可用串口:")
        for port in ports:
            print(f"  {port.device} - {port.description}")
            if 'CH340' in port.description or 'CP210' in port.description or 'ESP32' in port.description:
                print(f"  ↑ 这可能是ESP32！")
                return port.device
        return None

    def connect_serial(self):
        """连接ESP32串口"""
        if self.serial_port is None:
            self.serial_port = self.find_esp32_port()
        
        if self.serial_port is None:
            print("未找到ESP32串口，请手动指定端口号")
            return False
        
        try:
            self.serial = serial.Serial(self.serial_port, self.baud_rate, timeout=1)
            time.sleep(2)  # 等待ESP32复位
            print(f"已连接到ESP32: {self.serial_port}")
            return True
        except Exception as e:
            print(f"串口连接失败: {e}")
            return False

    def open_camera(self):
        """打开摄像头"""
        self.cap = cv2.VideoCapture(self.camera_index)
        if not self.cap.isOpened():
            print(f"错误：无法打开摄像头 {self.camera_index}")
            return False
        print(f"摄像头 {self.camera_index} 已打开")
        return True

    def capture_and_detect(self):
        """拍照并进行检测"""
        ret, frame = self.cap.read()
        if not ret:
            print("无法读取摄像头画面")
            return None
        
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
                    'confidence': round(conf, 3),
                    'bbox': [round(x1), round(y1), round(x2), round(y2)]
                })
        
        return detections

    def send_to_esp32(self, data):
        """发送数据到ESP32"""
        if self.serial and self.serial.is_open:
            msg = json.dumps(data) + '\n'
            self.serial.write(msg.encode('utf-8'))
            print(f"发送到ESP32: {data}")

    def read_from_esp32(self):
        """读取ESP32发来的指令"""
        if self.serial and self.serial.is_open and self.serial.in_waiting > 0:
            line = self.serial.readline().decode('utf-8').strip()
            return line
        return None

    def start_service(self):
        """启动检测服务"""
        print("\n=== ESP32 YOLO检测服务 ===")
        print("等待ESP32指令...")
        print("指令: 'DETECT' - 检测当前画面")
        print("指令: 'CHECK'  - 检查连接")
        print("按 Ctrl+C 退出\n")
        
        if not self.connect_serial():
            return
        
        if not self.open_camera():
            return
        
        self.running = True
        
        try:
            while self.running:
                cmd = self.read_from_esp32()
                
                if cmd:
                    print(f"收到ESP32指令: {cmd}")
                    
                    if cmd == 'DETECT':
                        detections = self.capture_and_detect()
                        
                        if detections is None:
                            self.send_to_esp32({'status': 'error', 'message': 'capture failed'})
                        elif len(detections) == 0:
                            self.send_to_esp32({'status': 'ok', 'count': 0, 'objects': []})
                            print("未检测到物体")
                        else:
                            result = {
                                'status': 'ok',
                                'count': len(detections),
                                'objects': detections
                            }
                            self.send_to_esp32(result)
                            for obj in detections:
                                print(f"  检测到: {obj['class']}, 置信度: {obj['confidence']}")
                    
                    elif cmd == 'CHECK':
                        self.send_to_esp32({'status': 'ok', 'message': 'connected'})
                    
                    else:
                        self.send_to_esp32({'status': 'error', 'message': f'unknown command: {cmd}'})
                
                time.sleep(0.01)
                
        except KeyboardInterrupt:
            print("\n服务已停止")
        finally:
            self.cleanup()

    def cleanup(self):
        """清理资源"""
        self.running = False
        if self.cap:
            self.cap.release()
        if self.serial and self.serial.is_open:
            self.serial.close()
        print("资源已释放")

    def list_ports(self):
        """列出所有可用串口（Windows用）"""
        ports = serial.tools.list_ports.comports()
        print("\n=== 可用串口列表 ===")
        for port in ports:
            print(f"  {port.device} - {port.description}")
        if not ports:
            print("  未找到任何串口设备")
        print("\nESP32通常显示为 'CH340' 或 'CP210x' 或 'Silicon Labs'")

if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='ESP32 YOLO检测服务')
    parser.add_argument('--port', type=str, default=None, help='ESP32串口号 (如 COM3)')
    parser.add_argument('--baud', type=int, default=115200, help='波特率')
    parser.add_argument('--camera', type=int, default=0, help='摄像头索引')
    parser.add_argument('--conf', type=float, default=0.3, help='置信度阈值')
    parser.add_argument('--list-ports', action='store_true', help='列出所有可用串口')
    parser.add_argument('--model', type=str, default='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt', 
                        help='模型路径')
    
    args = parser.parse_args()
    
    service = ESP32DetectionService(
        model_path=args.model,
        serial_port=args.port,
        baud_rate=args.baud,
        camera_index=args.camera,
        conf_threshold=args.conf
    )
    
    if args.list_ports:
        service.list_ports()
    else:
        service.start_service()
