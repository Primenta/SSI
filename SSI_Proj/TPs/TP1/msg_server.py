# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import os
import datetime
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography import x509
import datetime
import cert

conn_cnt = 0
conn_port = 8443
max_msg_size = 9999
userList = {}

p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
g = 2


pn = dh.DHParameterNumbers(p,g)
Parameters = pn.parameters()

class ServerWorker:
    """ Class that implements server functionality. """
    def __init__(self, cnt, addr=None, key=None):   
        """ Class constructor. """
        self.id = cnt
        self.addr = addr
        self.msg_cnt = 0
        self.pk_DH = Parameters.generate_private_key()
        self.pk_RSA = self.load_private_key("MSG_SERVER.key")
        self.pubKey_ClientDH = None
        self.aesgcm = None

        self.uid = ""
        self.subject = ""
        self.message_queue = {}

    def parseCert(self, subjectCert):
        subject = {}
        for attr in subjectCert:
            subject[attr.oid._name] = attr.value
        
        return subject

    def load_private_key(self, filename):
        """Upload the private key from the file."""
        with open(filename, 'rb') as key_file:
            key_data = key_file.read()
        
        private_key = serialization.load_pem_private_key(
            key_data,
            password=b"1234", 
            backend=default_backend()
        )
        return private_key

    
    def send(self, uid, subject, message):
        if uid not in userList:
            userList[uid] = {'message_queue': []}
            
        if not userList[uid]['message_queue']:
            index = 1
        else:
            index = len(userList[uid]['message_queue']) + 1

        time = datetime.datetime.now()
        entry = f"{index}:100758:{time}:{subject}:{message}"
        userList[uid]['message_queue'].append(entry)

        print("userList updated:", userList)

        return "Message sent"

    def askqueue(self, uid):
        if uid in userList:
            message_queue_str = "\n".join(userList[uid]['message_queue'])
            return message_queue_str
        else:
            return "Doesn't Exist"
    
    def getmsg(self, num):
        uid = self.uid
        if uid in userList:
            message_queue = userList[uid]['message_queue']
            for message in message_queue:
                parts = message.split(':')
                if len(parts) >= 5 and parts[0] == num:  
                    return ":".join(parts[4:]).encode('utf-8')  
        return b"Message not found" 

    def process(self, msg):
        """ Processes a message (`bytestring`) sent by the Client.
            Returns the message to transmit as a response (`None` for
            end call) """
        self.msg_cnt += 1
        
        if self.msg_cnt == 1:
            try:
                # Deserialize the public key from the received message
                pub_key = serialization.load_pem_public_key(
                    msg,
                    backend=default_backend()
                )
            except Exception as e:
                print(f"Error deserializing public key: {e}")
                return None
            
            self.pubKey_ClientDH = pub_key

            pub_key = self.pk_DH.public_key()
            pub_key_serialized = pub_key.public_bytes(
                                 encoding=serialization.Encoding.PEM,
                                 format=serialization.PublicFormat.SubjectPublicKeyInfo
                                 )
            
            pair_pubkeys = cert.mkpair(pub_key_serialized,msg)
            
            signServer = self.pk_RSA.sign(
                            pair_pubkeys,
                            padding.PSS(
                                mgf=padding.MGF1(hashes.SHA256()),
                                salt_length=padding.PSS.MAX_LENGTH
                            ),
                            hashes.SHA256()
                        )
            

            new_msg = cert.mkpair(cert.mkpair(pub_key_serialized, signServer), cert.serialize_certificate(cert.cert_load("MSG_SERVER.crt")))

        elif self.msg_cnt == 2:
            signClient, certClient_serialized = cert.unpair(msg)

            certClient = cert.deserialize_certificate(certClient_serialized)


            pub_key = self.pk_DH.public_key()
            pub_key_serialized_Server = pub_key.public_bytes(
                                 encoding=serialization.Encoding.PEM,
                                 format=serialization.PublicFormat.SubjectPublicKeyInfo
                                 )
            
            pub_key_serialized_Client = self.pubKey_ClientDH.public_bytes(
                                        encoding=serialization.Encoding.PEM,
                                        format=serialization.PublicFormat.SubjectPublicKeyInfo
                                        )
            
            pair_pubkeys = cert.mkpair(pub_key_serialized_Server,pub_key_serialized_Client)

            ca_crt = cert.cert_load("MSG_CA.crt")

            print(self.parseCert(certClient.subject)["pseudonym"])

            if(cert.valida_certALICE(ca_crt ,certClient)):
                pubkey_ClientRSA = certClient.public_key()

                pubkey_ClientRSA.verify( signClient,
                        pair_pubkeys,        
                        padding.PSS(
                        mgf=padding.MGF1(hashes.SHA256()),
                        salt_length=padding.PSS.MAX_LENGTH
                        ),
                        hashes.SHA256()
                )

                # Perform key exchange
                shared_key = self.pk_DH.exchange(self.pubKey_ClientDH)

                # Perform key derivation.
                derived_key = HKDF(
                    algorithm=hashes.SHA256(),
                    length=32,
                    salt=None,
                    info=b'handshake data',
                ).derive(shared_key)

                self.aesgcm = AESGCM(derived_key)

                txt = "Sucess!"
                new_msg = txt.upper().encode()
                rnd = os.urandom(12)
                encrytxt = self.aesgcm.encrypt(rnd, new_msg, None)
                new_msg = rnd + encrytxt
              
            elif txt.startswith("message"):
                partes = txt.split()
                message = ' '.join(partes[1:])
                self.send(self.uid, self.subject, message)
                response = b"Message sent successfully."
                
            else: 
                print("Error validating the certificate!")
                new_msg = ""

        else:
            rnd = msg[:12]
            encrytxt = msg[12:]
            

            try:
                plaintext = self.aesgcm.decrypt(rnd, encrytxt, None)
                txt = plaintext.decode().strip()
                msgA = txt.split()

                if txt.startswith("user"):

                    if len(msgA) == 1:
                        userFile = "userdata.p12"
                    else:
                        userFile = msgA[1]

                    if os.path.exists(userFile):
                        response = b"User file specified successfully."
                    else:
                        response = b"User file specified unsuccessfully."
                    
                elif txt.startswith("send"):
                    self.uid = msgA[1]
                    self.subject = " ".join(msgA[2:])

                    print(f"Received <send> command from client: send {self.uid} {self.subject}")
                    response = b"Write your message:"

                elif txt.startswith("message"):
                    print("Message received: " + txt)
                    partes = txt.split()
                    message = ' '.join(partes[1:])
                    print("vou para a send")
                    self.send(self.uid, self.subject, message)
                    print("saí da send")
                    response = b"Message sent successfully."

                elif txt.startswith("askqueue"):
                    if len(msgA) != 1:
                        response = b"Invalid number of arguments for askqueue"
                    else:
                        response = b"Messages on user with UID 1:" + self.askqueue("1").encode()
                    
                elif txt.startswith("getmsg"):
                    if len(msgA) != 2:
                        response = b"Invalid number of arguments for getmsg."
                    else:
                        num = msgA[1]
                        result = self.getmsg(num)  # assumindo que getmsg já retorna bytes
                        response = b"Message content for number " + num.encode() + b":" + result
                    
                elif txt.startswith("help"):
                    response = b"List commands:||send <UID> <SUBJECT>||askqueue||getmsg <NUM>"
                    
                else:
                    response = b"Invalid command."
            except Exception as e:
                print(f"Error decrypting message: {e}")
                return None

            txt = response.decode()
            print('%d: %r' % (self.id, txt))
            new_msg = txt.upper().encode()
            
            rnd = os.urandom(12)
            encrytxt = self.aesgcm.encrypt(rnd, new_msg, None)
            new_msg = rnd + encrytxt

            pass
        
        return new_msg if len(new_msg) > 0 else None
        

#
#
# Funcionalidade Cliente/Servidor
#
# obs: não deverá ser necessário alterar o que se segue
#
async def handle_echo(reader, writer):
    global conn_cnt
    conn_cnt +=1
    addr = writer.get_extra_info('peername')
    srvwrk = ServerWorker(conn_cnt, addr)
    data = await reader.read(max_msg_size)
    while True:
        if not data: continue
        if data[:1]==b'\n': break
        data = srvwrk.process(data)
        if not data: break
        writer.write(data)
        await writer.drain()
        data = await reader.read(max_msg_size)
    print("[%d]" % srvwrk.id)
    writer.close()


def run_server():
    loop = asyncio.new_event_loop()
    coro = asyncio.start_server(handle_echo, '127.0.0.1', 12340)
    server = loop.run_until_complete(coro)
    # Serve requests until Ctrl+C is pressed
    print('Serving on {}'.format(server.sockets[0].getsockname()))
    print('  (type ^C to finish)\n')
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass
    # Close the server
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()
    print('\nFINISHED!')

run_server()
