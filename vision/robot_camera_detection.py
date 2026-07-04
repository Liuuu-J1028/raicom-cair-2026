from ultralytics import YOLO
import cv2
import time

def run_camera_detection(model_path='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt', camera_index=0, conf_threshold=0.3):
    """
    使用机器人摄像头进行实时物体检测
    
    Args:
        model_path: 训练好的模型路径
        camera_index: 摄像头索引 (0表示默认摄像头)
        conf_threshold: 置信度阈值
    """
    print("正在加载模型...")
    model = YOLO(model_path)
    print("模型加载完成！")
    
    print(f"正在打开摄像头 {camera_index}...")
    cap = cv2.VideoCapture(camera_index)
    
    if not cap.isOpened():
        print(f"错误：无法打开摄像头 {camera_index}")
        return
    
    print("摄像头已打开！按 'q' 键退出")
    
    frame_count = 0
    start_time = time.time()
    
    while True:
        ret, frame = cap.read()
        
        if not ret:
            print("错误：无法读取摄像头帧")
            break
        
        results = model(frame, conf=conf_threshold)
        
        annotated_frame = results[0].plot()
        
        frame_count += 1
        current_time = time.time()
        fps = frame_count / (current_time - start_time)
        
        cv2.putText(annotated_frame, f"FPS: {fps:.1f}", (10, 30), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
        cv2.imshow('Robot Camera Detection', annotated_frame)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    cap.release()
    cv2.destroyAllWindows()
    print("检测结束！")

def run_video_file_detection(model_path='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt', video_path=None, conf_threshold=0.3):
    """
    使用视频文件进行检测（用于测试）
    
    Args:
        model_path: 训练好的模型路径
        video_path: 视频文件路径
        conf_threshold: 置信度阈值
    """
    print("正在加载模型...")
    model = YOLO(model_path)
    print("模型加载完成！")
    
    print(f"正在打开视频文件: {video_path}")
    cap = cv2.VideoCapture(video_path)
    
    if not cap.isOpened():
        print(f"错误：无法打开视频文件 {video_path}")
        return
    
    print("视频已打开！按 'q' 键退出")
    
    while True:
        ret, frame = cap.read()
        
        if not ret:
            print("视频播放完毕")
            break
        
        results = model(frame, conf=conf_threshold)
        annotated_frame = results[0].plot()
        
        cv2.imshow('Video Detection', annotated_frame)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    cap.release()
    cv2.destroyAllWindows()
    print("检测结束！")

if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='YOLO物体检测 - 机器人摄像头实时检测')
    parser.add_argument('--model', type=str, default='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt',
                        help='模型路径')
    parser.add_argument('--camera', type=int, default=0,
                        help='摄像头索引 (默认0)')
    parser.add_argument('--video', type=str, default=None,
                        help='视频文件路径（如果指定则使用视频文件而非摄像头）')
    parser.add_argument('--conf', type=float, default=0.3,
                        help='置信度阈值 (默认0.3)')
    
    args = parser.parse_args()
    
    if args.video:
        run_video_file_detection(args.model, args.video, args.conf)
    else:
        run_camera_detection(args.model, args.camera, args.conf)
