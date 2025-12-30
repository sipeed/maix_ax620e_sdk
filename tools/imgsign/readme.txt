spl_sign.py -i <spl_file> -pub <public_key_file(*.pem)> -prv <private_key_file(*.pem)> -o <out_file>
  -i   :  input original spl file path, the file size must be less than 111K bytes
  -pub :  input rsa public key file (*.pem) path
  -prv :  input rsa private key file (*.pem) path
  -o   :  output new spl file with 112K bytes

e.g.

python spl_sign.py -i fdl1.bin  -o fdl1_signed.bin -pub public.pem -prv private.pem

depends：
python 2.7 or python 3.x