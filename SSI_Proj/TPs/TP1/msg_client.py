# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import os
import sys
import socket
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import serialization

from cryptography.hazmat.primitives.asymmetric import padding
import cert
from cryptography.hazmat.primitives.serialization import pkcs12

max_msg_size = 9999

p = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF
g = 2

pn = dh.DHParameterNumbers(p,g)
Parameters = pn.parameters()


class Client:
    """ Class that implements the functionality of a Client. """
    def __init__(self, path, sckt=None):
        """ Class constructor. """
        self.sckt = sckt
        self.pathtofile = path
        self.msg_cnt = 0
        self.pk_DH = Parameters.generate_private_key()
        self.pk_RSA, self.cert_RSA, self.cert_CA = self.get_userdata(path)
        self.pubkey_Server = None
        self.aesgcm = None
    
    def get_userdata(self, p12_fname):
        with open(p12_fname, "rb") as f:
            p12 = f.read()
        password = None 
        (private_key, user_cert, [ca_cert]) = pkcs12.load_key_and_certificates(p12, password)
        return (private_key, user_cert, ca_cert)
    
    def newMessage(self, msg):
            print('Enter command:')
            
            return input()
        
    def invalidSendMessage(self, msg):
        msgA = msg.split()
        
        if len(msgA) < 3:
            return 'Invalid number of arguments for <send> command.'
        elif not msgA[1].isdigit():
            return 'Invalid UID.'        
        return None
    

    def process(self, msg=b""):
        self.msg_cnt +=1
        rnd = msg[:12]
        encrytxt = msg[12:] 

        
        if msg and self.msg_cnt > 2:
            plaintext = self.aesgcm.decrypt(rnd, encrytxt, None)
            txt = plaintext.decode()
            print('Received (%d): %r' % (self.msg_cnt , txt))

        
        if self.msg_cnt == 1:
            pub_key = self.pk_DH.public_key()
            new_msg = pub_key.public_bytes(
                        encoding=serialization.Encoding.PEM,
                        format=serialization.PublicFormat.SubjectPublicKeyInfo
                        )

        elif self.msg_cnt == 2:

            pair_PubKey_SignServer, certServer = cert.unpair(msg)

            pubkey_Server_serialized, signServer = cert.unpair(pair_PubKey_SignServer)
            
            deserialized_cert_server = cert.deserialize_certificate(certServer)

            if (cert.valida_certBOB(self.cert_CA, deserialized_cert_server)):
                try:
                    # Deserialize the public key from the received message
                    pubkey_ServerDH = serialization.load_pem_public_key(
                        pubkey_Server_serialized,
                        backend=default_backend()
                    )
                except Exception as e:
                    print(f"Error deserializing public key: {e}")
                    return None
                
                self.pubkey_Server = pubkey_ServerDH

                pubkey_ServerRSA = deserialized_cert_server.public_key()

                pub_key = self.pk_DH.public_key()
                pubKey_Client_serialized = pub_key.public_bytes(
                                                encoding=serialization.Encoding.PEM,
                                                format=serialization.PublicFormat.SubjectPublicKeyInfo
                                            )

                pair_pubkeys = cert.mkpair(pubkey_Server_serialized,pubKey_Client_serialized)

                pubkey_ServerRSA.verify(
                    signature=signServer,
                    data= pair_pubkeys,
                    padding=padding.PSS(
                        mgf=padding.MGF1(hashes.SHA256()),
                        salt_length=padding.PSS.MAX_LENGTH
                    ),
                    algorithm=hashes.SHA256()
                )


                signClient = self.pk_RSA.sign(
                            pair_pubkeys,
                            padding.PSS(
                                mgf=padding.MGF1(hashes.SHA256()),
                                salt_length=padding.PSS.MAX_LENGTH
                            ),
                            hashes.SHA256()
                        )
                
                new_msg = cert.mkpair(signClient,cert.serialize_certificate(self.cert_RSA))

                shared_key = self.pk_DH.exchange(pubkey_ServerDH)

                # Perform key derivation.
                derived_key = HKDF(
                    algorithm=hashes.SHA256(),
                    length=32,
                    salt=None,
                    info=b'handshake data',
                ).derive(shared_key)

                self.aesgcm = AESGCM(derived_key)

            else:
                print("ERRO na validação!")

                new_msg = ""

        elif txt.startswith("askqueue"):
            if len(msgA) != 1:
                response = b"Invalid number of arguments for askqueue"
            else:
                # Assuming UID is correctly extracted from the message
                uid = self.uid  # Assuming UID is extracted correctly
                response = self.askqueue(uid)

        else:
            msg = txt

            while True:
                if msg.startswith("WRITE YOUR MESSAGE"):
                    print("Please,write your message:")
                    new_txt = input()
                    new_msg = 'message ' + new_txt
                    break
                
                new_msg = self.newMessage(msg)

                if new_msg.startswith("send"):
                    if not (msg := self.invalidSendMessage(new_msg)): 
                        break

                elif not new_msg.startswith("Invalid"):
                    break

                else:
                    msg = "Invalid command."


            new_msg = new_msg.encode()

            if new_msg:
                rnd = os.urandom(12)
                encrytxt = self.aesgcm.encrypt(rnd, new_msg, None)
                new_msg = rnd + encrytxt
        

        return new_msg 

    def askQueue(self):
        print("Asking for queue...")
        # Assuming UID is correctly set before calling askQueue method
        uid_command = f"askqueue {self.uid}\n"
        self.sckt.sendall(uid_command.encode())
        data = self.sckt.recv(max_msg_size)
        print("Queue received:", data.decode())
        return data

#
#
# Funcionalidade Cliente/Servidor
#
# obs: não deverá ser necessário alterar o que se segue
#


async def tcp_echo_client():
    reader, writer = await asyncio.open_connection('127.0.0.1', 12340)
    addr = writer.get_extra_info('peername')

    if len(sys.argv) >= 3 and sys.argv[1] == "-user":
        pathtofile = sys.argv[2]
    else:
        pathtofile = "userdata.p12"

    client = Client(pathtofile, addr)

    msg = client.process()
    while msg:
        writer.write(msg)
        await writer.drain()
        msg = await reader.read(max_msg_size)
        if msg :
            msg = client.process(msg)
        else:
            break
    writer.write(b'\n')
    print('Socket closed!')
    writer.close()

def run_client():
    loop = asyncio.get_event_loop()
    loop.run_until_complete(tcp_echo_client())


run_client()
