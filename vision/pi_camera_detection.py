from ultralytics import YOLO
import cv2
import time

def run_pi_camera_detection(model_path='best.pt', resolution=(640, 480), conf_threshold=0.3):
    """
    树莓派摄像头实时检测（优化版）
    
    Args:
        model_path: 训练好的模型路径
        resolution: 摄像头分辨率
        conf_threshold: 置信度阈值
    """
    print("正在加载模型...")
    model = YOLO(model_path)
    print("模型加载完成！")
    
    print(f"正在打开摄像头，分辨率: {resolution}")
    
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])
    
    if not cap.isOpened():
        print("错误：无法打开摄像头")
        return
    
    print("摄像头已打开！按 'q' 键退出")
    
    frame_count = 0
    start_time = time.time()
    
    while True:
        ret, frame = cap.read()
        
        if not ret:
            print("错误：无法读取摄像头帧")
            break
        
        results = model(frame, conf=conf_threshold, imgsz=640)
        
        annotated_frame = results[0].plot()
        
        frame_count += 1
        current_time = time.time()
        fps = frame_count / (current_time - start_time)
        
        cv2.putText(annotated_frame, f"FPS: {fps:.1f}", (10, 30), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
        cv2.imshow('Pi Camera Detection', annotated_frame)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    cap.release()
    cv2.destroyAllWindows()
    print("检测结束！")

def run_pi_camera_no_display(model_path='best.pt', resolution=(640, 480), conf_threshold=0.3):
    """
    树莓派摄像头检测（无显示模式，适合无头运行）
    
    Args:
        model_path: 训练好的模型路径
        resolution: 摄像头分辨率
        conf_threshold: 置信度阈值
    """
    print("正在加载模型...")
    model = YOLO(model_path)
    print("模型加载完成！")
    
    print(f"正在打开摄像头，分辨率: {resolution}")
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])
    
    if not cap.isOpened():
        print("错误：无法打开摄像头")
        return
    
    print("摄像头已打开！按 Ctrl+C 退出")
    
    try:
        while True:
            ret, frame = cap.read()
            
            if not ret:
                print("错误：无法读取摄像头帧")
                break
            
            results = model(frame, conf=conf_threshold, imgsz=640)
            
            for result in results:
                for box in result.boxes:
                    cls = box.cls[0]
                    conf = box.conf[0]
                    class_name = model.names[int(cls)]
                    print(f"检测到: {class_name}, 置信度: {conf:.2f}")
            
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\n检测结束！")
        cap.release()

if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='树莓派YOLO物体检测')
    parser.add_argument('--model', type=str, default='best.pt', help='模型路径')
    parser.add_argument('--width', type=int, default=640, help='摄像头宽度')
    parser.add_argument('--height', type=int, default=480, help='摄像头高度')
    parser.add_argument('--conf', type=float, default=0.3, help='置信度阈值')
    parser.add_argument('--no-display', action='store_true', help='无显示模式')
    
    args = parser.parse_args()
    
    if args.no_display:
        run_pi_camera_no_display(args.model, (args.width, args.height), args.conf)
    else:
        run_pi_camera_detection(args.model, (args.width, args.height), args.conf)
