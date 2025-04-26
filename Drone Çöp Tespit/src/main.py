import warnings
warnings.filterwarnings('ignore', category=FutureWarning)

import cv2
import torch

# ─── Configuration ────────────────────────────────────────────────────────────
MODEL_NAME   = "yolov5l"
CAMERA_SRC   = 0            # For RTSP stream, "rtsp://username:password@ip_address:port/stream"
FRAME_WIDTH  = 640
FRAME_HEIGHT = 480

# ─── Model Loading ───────────────────────────────────────────────────────────
def load_model(name: str = MODEL_NAME):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = torch.hub.load("ultralytics/yolov5", name, pretrained=True)
    return model.to(device).eval(), device

# ─── Main Loop ────────────────────────────────────────────────────────────────
def main():
    model, device = load_model()
    cap = cv2.VideoCapture(CAMERA_SRC, cv2.CAP_DSHOW)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)

    while True:
        ret, frame = cap.read()
        if not ret:
            continue
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        with torch.no_grad():
            preds = model(rgb)
            # Print detected classes in the terminal
            for *box, conf, cls in preds.xyxy[0]:
                class_name = model.names[int(cls)]
                print(f"Detected: {class_name} (confidence: {conf:.2f})")
            annotated = preds.render()[0]
        frame_out = cv2.cvtColor(annotated, cv2.COLOR_RGB2BGR)
        cv2.imshow("YOLOv5 Live", frame_out)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()