import threading


class StoppableThread(threading.Thread):
    worker = None
    def __init__(self, w):        
        threading.Thread.__init__(self)
        self.worker = w

    def run(self):
        self.alive = True
        def c():
            return self.shouldStop()
        self.worker(c)

    def shouldStop(self):
        return not self.alive

    def stop(self):
        self.alive = False
        self.join()

