from ultralytics import YOLO
import os
import glob

def auto_label_images():
    """使用预训练模型自动检测并生成YOLO格式标签文件"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    train_images_dir = os.path.join(script_dir, 'dataset', 'train', 'images')
    train_labels_dir = os.path.join(script_dir, 'dataset', 'train', 'labels')
    valid_images_dir = os.path.join(script_dir, 'dataset', 'valid', 'images')
    valid_labels_dir = os.path.join(script_dir, 'dataset', 'valid', 'labels')

    os.makedirs(train_labels_dir, exist_ok=True)
    os.makedirs(valid_labels_dir, exist_ok=True)

    # 加载预训练模型
    print("加载 YOLOv8n 预训练模型...")
    model = YOLO('yolov8n.pt')

    def process_images(images_dir, labels_dir, dataset_name):
        image_files = glob.glob(os.path.join(images_dir, '*.jpg')) + \
                      glob.glob(os.path.join(images_dir, '*.jpeg')) + \
                      glob.glob(os.path.join(images_dir, '*.png'))

        image_files = [f for f in image_files if 'detected_' not in os.path.basename(f)]

        print(f"\n处理 {dataset_name} 集，共 {len(image_files)} 张图片...")

        labeled_count = 0
        for img_path in image_files:
            img_name = os.path.basename(img_path)
            label_name = os.path.splitext(img_name)[0] + '.txt'
            label_path = os.path.join(labels_dir, label_name)

            print(f"\n处理: {img_name}")

            results = model(img_path, verbose=False)

            with open(label_path, 'w') as f:
                for result in results:
                    boxes = result.boxes
                    if len(boxes) == 0:
                        print(f"  未检测到物体")
                        continue

                    for box in boxes:
                        cls = int(box.cls[0])
                        # 将 COCO 类别映射到我们的类别
                        # 7=teddy bear, 15=person, 16=bird, 17=cat, 18=dog, 19=elf, 56=toothbrush
                        # 齿轮可能映射为 7(teddy bear) 或其他
                        # 螺栓可能映射为其他类别

                        # 获取边界框（归一化坐标）
                        x1, y1, x2, y2 = box.xyxy[0]
                        conf = float(box.conf[0])

                        # 计算 YOLO 格式：x_center, y_center, width, height
                        img_h, img_w = result.orig_shape
                        x_center = ((x1 + x2) / 2) / img_w
                        y_center = ((y1 + y2) / 2) / img_h
                        width = (x2 - x1) / img_w
                        height = (y2 - y1) / img_h

                        # 映射到我们定义的类别（0=gear, 1=bolt）
                        # 这里我们简单地使用检测到的类别
                        mapped_cls = cls % 2  # 简单地交替映射

                        f.write(f"{mapped_cls} {x_center:.6f} {y_center:.6f} {width:.6f} {height:.6f}\n")
                        print(f"  检测到物体: 类别={mapped_cls}, 置信度={conf:.2f}, 位置=({x_center:.3f}, {y_center:.3f})")

            labeled_count += 1

        print(f"\n{dataset_name} 集完成！已处理 {labeled_count} 张图片")

    process_images(train_images_dir, train_labels_dir, "训练")
    process_images(valid_images_dir, valid_labels_dir, "验证")

    print("\n" + "="*50)
    print("标签文件生成完成！")
    print("="*50)

if __name__ == '__main__':
    auto_label_images()
