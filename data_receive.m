client = tcpclient('127.0.0.1',9090);
configureCallback(client,'byte',8,@bytes_read);