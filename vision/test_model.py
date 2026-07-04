from ultralytics import YOLO
import cv2
import os

def detect_with_custom_model(image_path, model_path='runs/detect/gear_bolt_detector_bolt_focused/weights/best.pt', conf_threshold=0.1):
    """使用训练好的自定义模型进行检测"""
    model = YOLO(model_path)
    
    results = model(image_path, conf=conf_threshold)
    
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
    test_images = [
        'dataset/train/images/t型螺栓.jpg',
        'dataset/train/images/齿轮.jpg',
        'dataset/train/images/t型螺栓_1.jpg',
        'dataset/train/images/齿轮_1.jpg',
        'dataset/train/images/t型螺栓_2.jpg',
        'dataset/train/images/齿轮_2.jpg',
        'dataset/train/images/t型螺栓_3.jpg',
        'dataset/train/images/齿轮_3.jpg'
    ]
    
    for img_path in test_images:
        if os.path.exists(img_path):
            detect_with_custom_model(img_path)
        else:
            print(f"错误：图片不存在: {img_path}")
    
    print("\n检测完成！")
