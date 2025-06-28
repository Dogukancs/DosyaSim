Kodlar windows sistem üzerinde vscode idesi ile yazıldı, VirtualMachine üzerinde ubuntu sistemdeki terminalde gerekli kurulumlar yapılarak makefile dosyasının çalıştırılması gerekli.
Shared dosya şeklinde iki taraflı sistem yapıldı. Virtual makine üzerinde shared klasörüne gelen proje için 

ubuntu terminalde: 
cd ~/shared
make
./simplefs

şeklinde yazılarak program çalıştırılır.

programın işi bittiğinde (oluşan .o dosyaları veya diğer dosyaların silinmesi için):
 
make clean 

komutu yeterli olacaktır.

