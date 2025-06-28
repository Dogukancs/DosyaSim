#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

void print_menu(void) {
    puts("\nBasit Dosya Sistemi Simülatörü (SimpleFS)");
    puts("1. Dosya oluştur");
    puts("2. Dosya sil");
    puts("3. Dosyaya veri yaz");
    puts("4. Dosyadan veri oku");
    puts("5. Dosyaları listele");
    puts("6. Format at");
    puts("7. Dosya ismini değiştir");
    puts("8. Dosya var mı kontrol et");
    puts("9. Dosya boyutunu göster");
    puts("10. Dosyaya veri ekle");
    puts("11. Dosya içeriğini göster (cat)");
    puts("12. Dosyayı kes (truncate)");
    puts("13. Dosya kopyala");
    puts("14. Dosya taşı (mv)");
    puts("15. Disk defrag");
    puts("16. Disk bütünlük kontrolü");
    puts("17. Disk yedeği oluştur");
    puts("18. Disk yedeğini geri yükle");
    puts("19. Çıkış");
    fputs("Seçiminiz: ", stdout);
}

int main(void) {
    int choice;
    char fn1[FILENAME_MAXLEN], fn2[FILENAME_MAXLEN];
    char data[1024];
    int size, offset;

    if (read_metadata() < 0) {
        puts("Disk metadata okunamadı, format uygulanıyor.");
        if (fs_format() != 0) {
            fprintf(stderr, "Format başarısız. Çıkılıyor.\n");
            return 1;
        }
    }

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            puts("Geçersiz seçim!"); while (getchar()!='\n');
            continue;
        }
        while (getchar()!='\n');

        switch (choice) {
            case 1:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                fs_create(fn1)==0?puts("Oluşturuldu"):puts("Hata");
                break;
            case 2:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                fs_delete(fn1)==0?puts("Silindi"):puts("Hata");
                break;
            case 3:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                printf("Veri: "); fgets(data, sizeof(data), stdin);
                data[strcspn(data, "\n")] = '\0';
                fs_write(fn1, data, strlen(data))==0?puts("Yazıldı"):puts("Hata");
                break;
            case 4:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                printf("Offset: "); scanf("%d", &offset);
                printf("Boyut: "); scanf("%d", &size); while(getchar()!='\n');
                {
                    char *buf=malloc(size+1);
                    int r=fs_read(fn1, offset, size, buf);
                    if (r>=0) { buf[r]='\0'; puts(buf); }
                    else puts("Hata");
                    free(buf);
                }
                break;
            case 5: fs_ls(); break;
            case 6: fs_format()==0?puts("Formatlandı"):puts("Hata"); break;
            case 7:
                printf("Eski ad: "); scanf("%31s", fn1); while(getchar()!='\n');
                printf("Yeni ad: "); scanf("%31s", fn2); while(getchar()!='\n');
                fs_rename(fn1, fn2)==0?puts("Değiştirildi"):puts("Hata");
                break;
            case 8:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                fs_exists(fn1)?puts("Var"):puts("Yok");
                break;
            case 9:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                printf("Boyut: %d\n", fs_size(fn1));
                break;
            case 10:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                printf("Eklenecek veri: "); fgets(data,sizeof(data),stdin);
                data[strcspn(data,"\n")]=0;
                fs_append(fn1,data,strlen(data))==0?puts("Eklendi"):puts("Hata");
                break;
            case 11:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                fs_cat(fn1); break;
            case 12:
                printf("Dosya adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                printf("Yeni boyut: "); scanf("%d",&size); while(getchar()!='\n');
                fs_truncate(fn1,size)==0?puts("Kesildi"):puts("Hata");
                break;
            case 13:
                printf("Kaynak: "); scanf("%31s", fn1); while(getchar()!='\n');
                printf("Hedef: "); scanf("%31s", fn2); while(getchar()!='\n');
                fs_copy(fn1,fn2)==0?puts("Kopyalandı"):puts("Hata");
                break;
            case 14:
                printf("Eski: "); scanf("%31s", fn1); while(getchar()!='\n');
                printf("Yeni: "); scanf("%31s", fn2); while(getchar()!='\n');
                fs_mv(fn1,fn2)==0?puts("Taşındı"):puts("Hata");
                break;
            case 15: fs_defragment()==0?puts("Defrag OK"):puts("Hata"); break;
            case 16: fs_check_integrity()==0?puts("OK"):puts("Tutarsız"); break;
            case 17:
                printf("Yedek adı: "); scanf("%31s", fn1); while(getchar()!='\n');
                fs_backup(fn1)==0?puts("Yedeklendi"):puts("Hata");
                break;
            case 18:
                printf("Yükleme: "); scanf("%31s", fn1); while(getchar()!='\n');
                fs_restore(fn1)==0?puts("Yüklendi"):puts("Hata");
                break;
            case 19: return 0;
            default: puts("Geçersiz"); break;
        }
    }
}