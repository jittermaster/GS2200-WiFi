from socket import *
from chardet import detect 

## UDP受信クラス
class udprecv():
    def __init__(self):

        SrcIP = "192.168.1.11"                             # 受信元IP
        SrcPort = 10001                                 # 受信元ポート番号
        self.SrcAddr = (SrcIP, SrcPort)                 # アドレスをtupleに格納

        self.BUFSIZE = 1024                             # バッファサイズ指定
        self.udpServSock = socket(AF_INET, SOCK_DGRAM)  # ソケット作成
        self.udpServSock.bind(self.SrcAddr)             # 受信元アドレスでバインド

    def recv(self):
        while True:                                     # 常に受信待ち

            data, addr = self.udpServSock.recvfrom(self.BUFSIZE)                    # 受信
            self.udpServSock.sendto('Hello Spresense!!'.encode('utf-8'), addr)       # 送信
            print(data.decode('utf-8'), addr)                  # 受信データと送信アドレス表示


udp = udprecv()     # クラス呼び出し
udp.recv()          # 関数実行
