from ultralytics import YOLO
import os
import cv2
import numpy as np
from pathlib import Path

def augment_image(image_path, label_path, output_dir, num_augmented=5):
    """对单张图片进行数据增强"""
    img = cv2.imread(image_path)
    h, w = img.shape[:2]

    with open(label_path, 'r') as f:
        label_content = f.read().strip()

    img_name = Path(image_path).stem
    img_ext = Path(image_path).suffix

    aug_count = 0
    for i in range(num_augmented):
        aug_img = img.copy()

        # 1. 随机旋转 (90°, 180°, 270°)
        angle = np.random.choice([0, 90, 180, 270])
        if angle > 0:
            M = cv2.getRotationMatrix2D((w/2, h/2), angle, 1)
            aug_img = cv2.warpAffine(aug_img, M, (w, h))

        # 2. 随机水平翻转
        if np.random.rand() > 0.5:
            aug_img = cv2.flip(aug_img, 1)
            # 翻转标签中的 x_center
            lines = label_content.split('\n')
            new_lines = []
            for line in lines:
                parts = line.split()
                if len(parts) == 5:
                    cls, x, y, bw, bh = parts
                    x = f"{1 - float(x):.6f}"
                    new_lines.append(f"{cls} {x} {y} {bw} {bh}")
            label_content = '\n'.join(new_lines)

        # 3. 随机亮度调整
        brightness = np.random.uniform(0.7, 1.3)
        aug_img = cv2.convertScaleAbs(aug_img, alpha=brightness, beta=0)

        # 4. 随机对比度调整
        contrast = np.random.uniform(0.8, 1.2)
        aug_img = cv2.convertScaleAbs(aug_img, alpha=contrast, beta=0)

        # 5. 随机饱和度调整
        hsv = cv2.cvtColor(aug_img, cv2.COLOR_BGR2HSV)
        sat_scale = np.random.uniform(0.8, 1.2)
        hsv[:, :, 1] = np.clip(hsv[:, :, 1] * sat_scale, 0, 255).astype(np.uint8)
        aug_img = cv2.cvtColor(hsv, cv2.COLOR_HSV2BGR)

        # 保存增强后的图片和标签
        aug_name = f"{img_name}_aug{i+1}"
        aug_img_path = os.path.join(output_dir, f"{aug_name}{img_ext}")
        aug_label_path = os.path.join(output_dir, f"{aug_name}.txt")

        cv2.imwrite(aug_img_path, aug_img)
        with open(aug_label_path, 'w') as f:
            f.write(label_content)

        aug_count += 1
        print(f"  创建: {aug_name}{img_ext}")

    return aug_count

def main():
    train_images_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\train\images'
    train_labels_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\train\labels'

    image_files = [f for f in os.listdir(train_images_dir) if f.lower().endswith(('.jpg', '.png'))]
    image_files = [f for f in image_files if 'aug' not in f and 'detected' not in f and 'detection_result' not in f]

    print(f"找到 {len(image_files)} 张原始图片")
    print("开始数据增强...\n")

    total_created = 0
    for img_file in image_files:
        img_path = os.path.join(train_images_dir, img_file)
        label_path = os.path.join(train_labels_dir, os.path.splitext(img_file)[0] + '.txt')

        if not os.path.exists(label_path):
            print(f"  跳过 {img_file}（无标签文件）")
            continue

        print(f"处理: {img_file}")
        created = augment_image(img_path, label_path, train_images_dir, num_augmented=5)
        total_created += created

    print(f"\n数据增强完成！共创建 {total_created} 张增强图片")

if __name__ == '__main__':
    main()
