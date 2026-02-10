import cv2
import numpy as np
import urllib.request

# Camera URL
url = "http://192.168.0.53/cam.jpg"

# YOLO model files
weights_path = r"./YOLO/yolov3.weights"
config_path = r"./YOLO/yolov3.cfg"
names_path = r"./YOLO/coco.names"

# Load the YOLO model and COCO class names
net = cv2.dnn.readNet(weights_path, config_path)
with open(names_path, "r") as f:
    classes = [line.strip() for line in f.readlines()]

layer_names = net.getLayerNames()

# Handling the return value of getUnconnectedOutLayers()
out_layers = net.getUnconnectedOutLayers()
if isinstance(out_layers[0], list):
    output_layers = [layer_names[i[0] - 1] for i in out_layers]
else:
    output_layers = [layer_names[i - 1] for i in out_layers]

# Generate random colors for each class
colors = np.random.uniform(0, 255, size=(len(classes), 3))


def detect_objects(frame):
    height, width, _ = frame.shape
    blob = cv2.dnn.blobFromImage(frame, 1 / 255.0, (416, 416), swapRB=True, crop=False)
    net.setInput(blob)

    layer_outputs = net.forward(output_layers)

    boxes = []
    confidences = []
    class_ids = []

    for output in layer_outputs:
        for detection in output:
            scores = detection[5:]
            class_id = np.argmax(scores)
            confidence = scores[class_id]
            if confidence > 0.3:
                center_x = int(detection[0] * width)
                center_y = int(detection[1] * height)
                w = int(detection[2] * width)
                h = int(detection[3] * height)

                x = int(center_x - w / 2)
                y = int(center_y - h / 2)

                boxes.append([x, y, w, h])
                confidences.append(float(confidence))
                class_ids.append(class_id)

    indexes = cv2.dnn.NMSBoxes(boxes, confidences, 0.3, 0.4)

    # Draw detections on the frame
    if len(indexes) > 0 and isinstance(indexes, np.ndarray):
        indexes = indexes.flatten()
        for i in indexes:
            x, y, w, h = boxes[i]
            label = str(classes[class_ids[i]])
            confidence = confidences[i]
            color = colors[class_ids[i]]
            print(f"Detected: {label} with confidence {confidence:.2f}")

            cv2.rectangle(frame, (x, y), (x + w, y + h), color, 2)
            cv2.putText(
                frame,
                f"{label} {confidence:.2f}",
                (x, y - 10),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                color,
                2,
            )

    return frame


def main():
    cv2.namedWindow("Object Detection", cv2.WINDOW_AUTOSIZE)

    while True:
        try:
            img_resp = urllib.request.urlopen(url)
            imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
            frame = cv2.imdecode(imgnp, -1)
            frame = detect_objects(frame)

            cv2.imshow("Object Detection", frame)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                break
        except Exception as e:
            print(f"Error occurred: {e}")
            break

    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()