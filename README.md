# EncryptedWifiMessenger

A ChipKit peer with a pic32 processor and Uno32 IO shield that can communicate with another peer.
The Chipkit does not have a OS and does not have the C standard library so all needed operations have been implemented.
The communication starts with a diffie hellman key exchange and then the messages are encrypted and decrypted
with chacha20.

The communication uses the SCS protocol.

Mcb32tools are used.
