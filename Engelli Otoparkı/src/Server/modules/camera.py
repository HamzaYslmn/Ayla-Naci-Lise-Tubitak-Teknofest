import sys

if sys.platform.startswith("win"):
    import cv2

    def get_camera():
        cap = cv2.VideoCapture(0)
        detector = cv2.QRCodeDetector()
        return cap, detector

    def show_qr_frame(cap, detector):
        ret, frame = cap.read()
        if not ret:
            return None
        data, points, _ = detector.detectAndDecode(frame)
        if points is not None:
            pts = points[0].astype(int)
            for j in range(len(pts)):
                cv2.line(frame, tuple(pts[j]), tuple(pts[(j+1)%len(pts)]), (0,255,0), 3)
        cv2.imshow("QR Code Scanner", frame)
        key = cv2.waitKey(1000 if data else 1)
        if key & 0xFF == ord('q'):
            return "quit"
        return data or None

    def release_camera(cap):
        cap.release()
        cv2.destroyAllWindows()

else:
    from picamera2 import Picamera2
    import cv2

    def get_camera():
        picam = Picamera2()
        picam.start()
        detector = cv2.QRCodeDetector()
        return picam, detector

    def show_qr_frame(picam, detector):
        frame = picam.capture_array()
        data, points, _ = detector.detectAndDecode(frame)
        if points is not None:
            pts = points[0].astype(int)
            for j in range(len(pts)):
                cv2.line(frame, tuple(pts[j]), tuple(pts[(j+1)%len(pts)]), (0,255,0), 3)
        cv2.imshow("QR Code Scanner", frame)
        key = cv2.waitKey(1000 if data else 1)
        if key & 0xFF == ord('q'):
            return "quit"
        return data or None

    def release_camera(picam):
        picam.close()
        cv2.destroyAllWindows()