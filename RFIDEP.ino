
 
#include <EEPROM.h>     //Okuma ve yazma işini UID ile EEPROM a kaydedicez.
#include <SPI.h>        //RC522 modülü SPI protokolünü kullanır.
#include <MFRC522.h>    //Mifare RC522 kütüphanesi

#define RESET 3//Master Card'ı silmek için buton pinini sseçtik.(Dilerseniz buton koymayın direk 3.pini GND ye verin.)
#define BUZER 2
#define SENSOR 5
#define sensorLed 6
boolean match=false;   
boolean programMode=false;  //Programlama modu başlatma.

int successRead;    //Başarılı bir şekilde sayı okuyabilmek için integer atıyoruz.
int sensorDeger = 0;
byte storedCard[4];   //Kart EEPROM tarafından okundu.
byte readCard[4];     //RFID modül ile ID tarandı.
byte masterCard[4];   //Master kart ID'si EEPROM'a aktarıldı.

#define SS_PIN 10
#define RST_PIN 8
MFRC522 mfrc522(SS_PIN,RST_PIN);




///////////////////////YÜKLEME/////////////////////

void setup()
{
    pinMode(RESET,INPUT_PULLUP);
    pinMode(BUZER,OUTPUT);
    digitalWrite(BUZER,LOW);
    pinMode(SENSOR,INPUT);
    pinMode(sensorLed, OUTPUT);
     sensorDeger=0;

  //Protokol konfigrasyonu
  Serial.begin(9600);    //PC ile seri iletim başlat.
  SPI.begin();           //MFRC522 donanımı SPI protokolünü kullanır.
  mfrc522.PCD_Init();    //MFRC522 donanımını başlat.

}
void loop()
{
  sensorDeger=digitalRead(SENSOR);
 
  if(sensorDeger==HIGH)
  {
    Start();
    digitalWrite(sensorLed,HIGH);
    delay(3000);
  do
  {
    //successRead=getID();
    //digitalWrite(BUZER,HIGH);  
    //SuccessRead 1 olursa kartı oku aksi halde 0
    for(int i = 0;i<5;i+=0.5)
    {
      successRead=getID();
     
      if(i>2)
      {
        digitalWrite(BUZER,HIGH);
      }
    }
  }
  while(!successRead);   
  if(programMode)
  {
    if(isMaster(readCard))    //Master Kart tekrar okutulursa programdan çıkar.
    {
      Serial.println(F("Master Kart Okundu.."));
      Serial.println(F("Programdan cikis yapiliyor.."));
      Serial.println(F("*********************"));
      programMode=false;
      return;
    }
    else
    {
      if(findID(readCard))   //Okunan kart silinmek isteniyorsa
      {
        Serial.println(F("Okunan kart siliniyor.."));
        deleteID(readCard);
        Serial.println(F("****************"));
      }
      else                   //Okunan kart kaydedilmek isteniyorsa
      {
        Serial.println(F("Okunan kart hafizaya kaydediliyor.."));
        writeID(readCard);
        Serial.println(F("**************************"));
      }
    }
  }
  else
  {
    if(isMaster(readCard))   //Master Kart okunursa programa giriş yapılıyor.
    {
      programMode=true;
      Serial.println(F("Merhaba Sayin TURK, Programa giris yapiyorum."));
      int count=EEPROM.read(0);
      Serial.print(F("Sahip oldugum kullanici "));
      Serial.print(count);
      Serial.print(F("sayisi kadardir."));
      Serial.println("");
      Serial.println(F("Eklemek veya cikarmak istediginiz karti okutunuz."));
      Serial.println(F("*****************************"));
    }
    else
    {
      if(findID(readCard))
      {
        Serial.println(F("Hosgeldin, gecis izni verildi."));
        granted(300);     //300 milisaniyede kilitli kapıyı aç.
      }
      else
      {
        Serial.println(F("Gecis izni verilmedi."));
        denied();
        
      }
    }
  }
  
  }
  else 
  {
    digitalWrite(sensorLed,LOW);
   
    }
}

////////////////////ERİŞİM İZNİ//////////////////////

void granted(int setDelay)
{
  Serial.println(F("BASARILI"));
  digitalWrite(BUZER,HIGH);
  delay(100);
  digitalWrite(BUZER,LOW);
  delay(100);
  digitalWrite(BUZER,HIGH);
  delay(100);
  digitalWrite(BUZER,LOW);
  delay(100);
  digitalWrite(BUZER,HIGH);
  delay(100);
  digitalWrite(BUZER,LOW); 
}


void Start()
{
  Serial.println(F("Giris Kontrol v1.0"));  //Hata ayıklama amacıyla


//  Silme kodu butona basıldığında EEPROM içindeki bilgileri siler.
  if(digitalRead(RESET)==LOW)
  {
    //Butona bastığınızda pini düşük almalısınız çünkü buton toprağa bağlı.
    Serial.println(F("Silme butonuna basildi."));
    Serial.println(F("5 saniye icinde iptal edebilirsiniz.."));
    Serial.println(F("Tum kayitlar silinecek ve bu islem geri alinamayacak."));
    delay(5000);                       //Kullanıcıya iptal işlemi için yeterli zaman verin.
    if (digitalRead(RESET)==LOW)       //Düğmeye basılmaya devam ediliyorsa EEPROM u sil.
    {
      Serial.println(F("EEPROM silinmeye baslaniyor."));
      
      for (int x=0; x<EEPROM.length();x=x+1)  //EEPROM adresinin döngü sonu
      {
        if(EEPROM.read(x)!=0)   //EEPROM adresi sıfır olursa
        {
          EEPROM.write(x,0);
        }
       }
       Serial.println(F("EEPROM Basariyla Silindi.."));
      
       }

       else
       {
        Serial.println(F("Silme islemi iptal edildi."));
       
       }
      }

/*Master Kart tanımlı mı kontrol edilecek.Eğer kullanıcı Master Kart seçmediyse      
  yeni bir Master Kart EEPROM a kaydedilecek.EEPROM kayıtlarının haricinde 143 EEPROM adresi tutulabilir.
  EEPROM adresi sihirli sayısı 143.
*/

 if (EEPROM.read(1) != 143)
 {
  Serial.println(F("Master Kart Secilmedi."));
  Serial.println(F("Master Karti secmek icin kartinizi okutunuz.."));
  do
  {
  successRead=getID();//successRead 1 olduğu zaman okuyucu düzenlenir aksi halde 0 olucaktır.

  }
  while(!successRead); 
  for (int j=0; j<4; j++)           //4 kez döngü
  {
    EEPROM.write(2+j,readCard[j]);    //UID EPPROM a yazıldı, 3. adres başla.
  }
  EEPROM.write(1,143);      //EEPROM a Master Kartı kaydettik.
  Serial.println(F("Master Kart kaydedildi.."));
  }

Serial.println(F("*****************"));
Serial.println(F("Master Kart UID:"));
for(int i=0; i<4; i++)                    //EEPROM da Master Kartın UID'si okundu.
{                                         //Master Kart yazıldı.
  masterCard[i]=EEPROM.read(2+i);
  Serial.print(masterCard[i],HEX);
}

Serial.println("");
Serial.println(F("***************"));
Serial.println(F("Hersey Hazir!!"));
Serial.println(F("Kart okutulmasi icin bekleniyor..."));
}

////////////////ERİŞİM İZNİ VERİLMEDİ/////////////////////
void denied()
{
  Serial.println(F("!!!!!BASARISIZ!!!!"));
  digitalWrite(BUZER,HIGH);
}

/////////////////////KART OKUYUCU UID AYARLAMA////////////////////////////

int getID()
{
  //Kart okuyucuyu hazır ediyoruz
  if(!mfrc522.PICC_IsNewCardPresent())   //yeni bir kart okutun vve devam edin.
  {
    return 0;
  }
  if(!mfrc522.PICC_ReadCardSerial())     //kartın serial numarasını alın ve devam edin.
  {
    return 0;
  }
  //4 ve 7 byte UID'ler mevcut biz 4 byte olanı kullanıcaz
  Serial.println(F("Kartin UID'sini taratin..."));
  for (int i=0; i<4; i++)
  {
    readCard[i]=mfrc522.uid.uidByte[i];
    Serial.print(readCard[i],HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); //Okuma durduruluyor.
  return 1;
}

///////////////////////EEPROM için ID Okuma////////////////////////

void readID( int number )
{
  int start=(number*4)+2;    //başlama pozisyonu
  for (int i=0; i<4; i++)     //4 byte alamabilmek için 4 kez döngü kurucaz
  {
    storedCard[i]=EEPROM.read(start+i); //  EEPROM dan diziye okunabilen değerler atayın.
  }
}

////////////////////EEPROM a ID Ekleme///////////////////////////////////

void writeID(byte a[])
{
  if (!findID(a))     //biz eeprom a yazmadan önce önceden yazılıp yazılmadığını kontrol edin.
  {
    int num=EEPROM.read(0);  
    int start= (num*4)+6;
    num++;
    EEPROM.write(0,num);
    for(int j=0;j<4;j++)
    {
      EEPROM.write(start+j,a[j]);
    }
    Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'a eklendi.."));
    }
    else
    {
      Serial.println(F("Basarisiz! Yanlıs ID"));
    }
}

/////////////////////////EEPROM'dan ID Silme////////////////////////

void deleteID(byte a[])
{
  if (!findID(a))       //Önceden EEPROM'dan silinen karta sahipmiyiz kontrol et.
  {
   Serial.println(F("Basarisiz! Yanlıs ID veya kotu EEPROM. :-("));
  }
  else
  {
    int num=EEPROM.read(0);
    int slot;
    int start;
    int looping;
    int j;
    int count=EEPROM.read(0);  //Kart numarasını saklayan ilk EEPROM'un ilk byte'ını oku
    slot=findIDSLOT(a);
    start=(slot*4)+2;
    looping=((num-slot)*4);
    num--;                            //tek sayacı azaltma
    EEPROM.write(0,num);
    for(j=0;j<looping;j++)
    {
      EEPROM.write(start+j,EEPROM.read(start+4+j));
    }
    for(int k=0;k<4;k++)
    {
      EEPROM.write(start+j+k,0);
    }
    Serial.println(F("Silme basarili."));
    Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'dan silinmistir.."));
  }
}


///////////////Byte'ların Kontrolü///////////////////

boolean checkTwo (byte a[],byte b[])
{
  if (a[0] != NULL)
  match=true;
  
  for (int k=0; k<4; k++)
  {
    if(a[k] != b[k])
    {
      match=false;
    }
    if(match)
    {
      return true;
    }
    else 
    {
      return false;
    }
  }
}

//////////////////////Boşluk Bulma////////////////////////////

int findIDSLOT (byte find[] )
{
  int count = EEPROM.read(0);     //EEPROM ile ilk byte ı okuyacağız.
  for(int i=1; i<=count; i++)      //Döngüdeki her EEPROM girişi için
  {
    readID(i);                  //EEPROM daki ID yi okuyacak ve Storedcard[4] de saklayacağız.
    if (checkTwo(find, storedCard))  // Saklı Kartlar da olup olmadığının kontrolü.
    { //aynı ID'e sahip kart bulursa geçişe izin vericek.
      return i;   //kartın slot numarası
      break;       //aramayı durdurucak.
    }
  }
}

///////////////////////EEPROM'da ID Bulma//////////////////////

boolean findID (byte find[] )
{
  int count = EEPROM.read(0);    //EEPROM'daki ilk byte'ı oku
  for(int i=1; i <= count; i++)    //Önceden giriş yapılmış mı kontrolü.
  {
    readID(i);      
    if(checkTwo(find,storedCard) )
    {
      return true;
      break;
    }
    else
    {
      //değilse return false
    }
  }
  return false;
}

////////////////Master Kartın Doğruluğunun Tespiti///////////////////

boolean isMaster (byte test[])
{
  if(checkTwo(test,masterCard))
  return true;
  else
  return false;
  
}








