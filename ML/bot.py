import ipaddress
import asyncio

class MyBot:

    def __init__(self, ip, port, token):
        self.ip = ip
        self.port = port
        self.token = token
    async def connect(self):
        reader, writer = await asyncio.open_connection(self.ip, self.port)
        self.writer = writer
        self.reader = reader
    async def send_msg(self, msg):
        writer = self.writer        
        writer.write(msg.encode())                                         # write msg in buffer, string --> bytes
        await writer.drain()                                               # waiting buffer to be cleared after sending
    async def run(self):
        await self.connect()
        await self.send_msg(f"/bot_reg {self.token}\n")
    async def disconnect(self):
        writer = self.writer
        writer.close()                                                     # close socket, but not immediately
        await writer.wait_closed()                                         # waiting end of closure
    def describe(self):
        print(f"ip = {self.ip}, port = {self.port}, token = {self.token}")

def main():

    port = 27010
    ip = "10.55.131.153"
    token = "adfd7429-0e16-450e-8acf-670a3346cae8"

    bot = MyBot(ip, port, token)

    asyncio.run(bot.run())                                                  # init Event Loop

if __name__ == "__main__":
    main()
