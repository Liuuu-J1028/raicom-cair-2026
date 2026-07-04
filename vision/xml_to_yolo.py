import os
import xml.etree.ElementTree as ET

def convert_xml_to_yolo(xml_path, output_dir, classes):
    """将 Pascal VOC 格式的 xml 文件转换为 YOLO 格式"""
    tree = ET.parse(xml_path)
    root = tree.getroot()

    img_width = int(root.find('size/width').text)
    img_height = int(root.find('size/height').text)

    yolo_lines = []
    for obj in root.findall('object'):
        cls_name = obj.find('name').text
        if cls_name not in classes:
            continue
        
        cls_id = classes.index(cls_name)
        
        bbox = obj.find('bndbox')
        xmin = int(bbox.find('xmin').text)
        ymin = int(bbox.find('ymin').text)
        xmax = int(bbox.find('xmax').text)
        ymax = int(bbox.find('ymax').text)

        x_center = (xmin + xmax) / 2 / img_width
        y_center = (ymin + ymax) / 2 / img_height
        width = (xmax - xmin) / img_width
        height = (ymax - ymin) / img_height

        yolo_lines.append(f"{cls_id} {x_center:.6f} {y_center:.6f} {width:.6f} {height:.6f}")

    if yolo_lines:
        base_name = os.path.splitext(os.path.basename(xml_path))[0]
        output_path = os.path.join(output_dir, base_name + '.txt')
        with open(output_path, 'w') as f:
            f.write('\n'.join(yolo_lines))
        print(f"已转换: {xml_path} -> {output_path}")

def batch_convert(xml_dir, output_dir, classes):
    os.makedirs(output_dir, exist_ok=True)
    
    xml_files = [f for f in os.listdir(xml_dir) if f.endswith('.xml')]
    
    for xml_file in xml_files:
        xml_path = os.path.join(xml_dir, xml_file)
        convert_xml_to_yolo(xml_path, output_dir, classes)
    
    print(f"\n转换完成！共处理 {len(xml_files)} 个文件")

if __name__ == '__main__':
    classes = ['gear', 'bolt']
    
    train_xml_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\train\labels'
    train_output_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\train\labels'
    
    valid_xml_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\valid\labels'
    valid_output_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\valid\labels'
    
    print("转换训练集标签...")
    batch_convert(train_xml_dir, train_output_dir, classes)
    
    print("\n转换验证集标签...")
    batch_convert(valid_xml_dir, valid_output_dir, classes)
