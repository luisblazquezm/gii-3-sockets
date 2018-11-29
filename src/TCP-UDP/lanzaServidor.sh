# lanzaServidor.sh
# Lanza el servidor que es un daemon y varios clientes
servidor
cliente olivo TCP r fichero1.txt &
cliente olivo TCP r fichero2.txt &
cliente olivo TCP l fichero3.txt &
cliente olivo UDP l fichero4.txt &
cliente olivo UDP r fichero5.txt &
cliente olivo UDP l fichero6.txt &


 