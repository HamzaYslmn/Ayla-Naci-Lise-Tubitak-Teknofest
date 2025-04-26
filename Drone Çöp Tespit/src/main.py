import cv2
import torch
import threading
import queue

# 0) Check if GPU is available and set device accordingly
version = torch.version.cuda
print(f"PyTorch version: {torch.__version__}")


device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"Using device: {device}")

# 1) Load model once
model = torch.hub.load("ultralytics/yolov5", "yolov5l", pretrained=True).to(device)

# 2) Shared structures
frame_q = queue.Queue(maxsize=1)
result_lock = threading.Lock()
result_frame = None
exit_event = threading.Event()

def camera_thread():
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH,  640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    while not exit_event.is_set():
        ret, frame = cap.read()
        if not ret:
            continue
        # Try to put the newest frame, drop old if still unprocessed
        try:
            frame_q.put_nowait(frame)
        except queue.Full:
            pass

    cap.release()

def inference_thread():
    global result_frame
    while not exit_event.is_set():
        try:
            frame = frame_q.get(timeout=0.03)
        except queue.Empty:
            continue

        # Run YOLOv5 inference
        preds = model(frame)
        annotated = preds.render()[0]

        # Store result
        with result_lock:
            result_frame = annotated

if __name__ == "__main__":
    # Start threads
    t_cam = threading.Thread(target=camera_thread, daemon=True)
    t_inf = threading.Thread(target=inference_thread, daemon=True)
    t_cam.start()
    t_inf.start()

    # Display loop
    while True:
        with result_lock:
            disp = result_frame

        if disp is not None:
            cv2.imshow("YOLOv5 Live", disp)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            exit_event.set()
            break

    cv2.destroyAllWindows()
