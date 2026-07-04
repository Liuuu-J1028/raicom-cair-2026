from ultralytics import YOLO
import cv2
import os

def detect_objects(image_path, model_name='yolov8n.pt'):
    """使用YOLO进行物体检测"""
    model = YOLO(model_name)
    
    results = model(image_path)
    
    print(f"\n=== 检测图片: {os.path.basename(image_path)} ===")
    print("检测结果:")
    
    has_detections = False
    for result in results:
        boxes = result.boxes
        if len(boxes) == 0:
            print("  未检测到任何物体")
            continue
            
        has_detections = True
        for box in boxes:
            x1, y1, x2, y2 = box.xyxy[0]
            conf = box.conf[0]
            cls = box.cls[0]
            class_name = model.names[int(cls)]
            
            print(f"  - {class_name}, 置信度: {conf:.2f}")
            print(f"    位置: ({int(x1)}, {int(y1)}) - ({int(x2)}, {int(y2)})")
    
    if has_detections:
        annotated_img = results[0].plot()
        output_name = f"detected_{os.path.basename(image_path)}"
        output_path = os.path.join(os.path.dirname(image_path), output_name)
        cv2.imwrite(output_path, annotated_img)
        print(f"  结果已保存到: {output_name}")

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    image_extensions = ['.jpg', '.jpeg', '.png', '.bmp']
    image_files = []
    
    # 搜索所有子目录中的图片
    for root, dirs, files in os.walk(script_dir):
        for f in files:
            if os.path.splitext(f)[1].lower() in image_extensions:
                image_files.append(os.path.join(root, f))
    
    if not image_files:
        print("错误：没有找到图片文件")
    else:
        print(f"找到 {len(image_files)} 张图片，开始检测...")
        for img_path in image_files:
            detect_objects(img_path)
        
        print("\n检测完成！")
