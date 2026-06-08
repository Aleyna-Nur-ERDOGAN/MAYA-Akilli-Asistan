// ============================================================
// MAYA ESP32 — JÜRİ MODU
// Jüri kartı (13376919) → BTN_2 → Python'dan Sheets çek
// → Nokia'da göster → BTN_3 ile seç → sesle söyle
// Pinler: LCD_DC=4, LCD_CE=16, LCD_RST=17
//         SPI: SCK=18,MISO=19,MOSI=23,SS=5  RFID RST=27
//         BTN1=22 BTN2=21 BTN3=14 BTN4=13 ACIL=32
// ============================================================

#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <time.h>

// ── WiFi & TELEGRAM ──────────────────────────────────────────
#define WIFI_SSID "MAYA"
#define WIFI_PASS "aleyna123"
#define BOTtoken  "8516599187:AAE9pUeVJ26EsVQpkpMIUqrXOWg_aWYSZPs"
#define CHAT_ID   "8589249980"

WiFiClientSecure     wifiClient;
UniversalTelegramBot bot(BOTtoken, wifiClient);

// ── EKRAN ────────────────────────────────────────────────────
#define LCD_DC  4
#define LCD_CE  16
#define LCD_RST 17
Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_DC, LCD_CE, LCD_RST);

// ── RFID ─────────────────────────────────────────────────────
#define SS_PIN  5
#define RST_PIN 27
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ── BUTONLAR ─────────────────────────────────────────────────
#define BTN_1    22   // Yukarı
#define BTN_2    21   // Aşağı
#define BTN_3    14   // Seç / Başlat-Durdur
#define BTN_4    13   // Menüye dön / Çıkış
#define BTN_ACIL 32   // SOS

// ── KİŞİSEL ──────────────────────────────────────────────────
const char* ogrenciAdi = "Aleyna Nur Erdogan";
const char* ogrenciNo  = "22040628";
const char* bolum      = "BOTE";

// ── RFID KARTLAR ─────────────────────────────────────────────
const String kartAleyna = "2372F10D";
const String kartJuri   = "13376919";

// ── SİSTEM DURUM ─────────────────────────────────────────────
bool   sistemAcik      = false;
bool   juriModu        = false;   // Jüri girişi yaptı mı?
int    aktifMod        = 0;       // 0:Menü 1:Oyun 2:Görev 3:Pomodoro
String aktifKullanici  = "";
bool   wifiBasli       = false;

// ── POMODORO & OYUN SAYACI ───────────────────────────────────
unsigned long zamSayac      = 0;
unsigned long oyunSayac     = 0;
int  kalanDak      = 25;
int  kalanSan      = 0;
int  oyunDak       = 45;
int  oyunSan       = 0;
bool durumModu     = true;    // true=Çalışma false=Mola
bool sayacCalis    = false;
bool oyunCalis     = false;

// ── GÖREV LİSTESİ ────────────────────────────────────────────
// Başlangıçta koda gömülü (offline yedek)
// Jüri girince Python'dan Sheets verisi gelir, bu liste güncellenir
const int TOPLAM = 12;
String gorevler[TOPLAM] = {
  "1.UIUX Tasarim",
  "2.VT Mimarisi ",
  "3.Ogun Kodlama",
  "4.Analiz Rapor",
  "5.ADDIE Strat.",
  "6.Storyboard  ",
  "7.Video Duzen ",
  "8.E-Ogrenme   ",
  "9.Mater.Tasar ",
  "10.Online Test",
  "11.Kullan.Test",
  "12.Pro.Rap.Haz"
};
int seciliIdx = 0;
int aktifIdx  = -1;
bool gorevListesiHazir = false;  // Python'dan liste geldi mi?

// ── WiFi İKONLARI ────────────────────────────────────────────
const unsigned char PROGMEM wifi_on[]  = {0x3C,0x42,0x99,0x24,0x42,0x18,0x18,0x00};
const unsigned char PROGMEM wifi_off[] = {0x3C,0x46,0x8D,0x34,0x5A,0x18,0x18,0x00};

// ============================================================
// YARDIMCI FONKSİYONLAR
// ============================================================

void sesCal(String k)  { Serial.println(k); }

bool buton(int pin) {
    if (digitalRead(pin) == LOW) {
        delay(40);
        if (digitalRead(pin) == LOW) {
            while (digitalRead(pin) == LOW);
            return true;
        }
    }
    return false;
}

String saatAl() {
    struct tm t;
    if (!getLocalTime(&t)) return "--:--";
    char buf[6];
    strftime(buf, 6, "%H:%M", &t);
    return String(buf);
}

// ── DURUM ÇUBUĞU ─────────────────────────────────────────────
void durum() {
    display.fillRect(0, 0, 84, 8, BLACK);
    display.setTextColor(WHITE, BLACK);
    display.setCursor(1, 0);
    display.print(wifiBasli ? "[ONL]" : "[OFF]");
    display.setCursor(32, 0);
    display.print(saatAl());
    display.drawBitmap(75, 0, wifiBasli ? wifi_on : wifi_off, 8, 8, WHITE);
    display.setTextColor(BLACK, WHITE);
}

// ── EKRAN FONKSİYONLARI ──────────────────────────────────────

void ekranUyku() {
    display.clearDisplay(); durum();
    display.setCursor(18,12); display.println("--    --");
    display.setCursor(34,22); display.println("zZz");
    display.setCursor(0, 34); display.println("MAYA UYUYOR");
    display.display();
}

void animasyon() {
    display.fillRoundRect(8, 14,18,14,4,BLACK);
    display.fillRoundRect(58,14,18,14,4,BLACK);
    display.fillCircle(12,18,2,WHITE);
    display.fillCircle(62,18,2,WHITE);
    display.drawCircleHelper(42,28,6,4,BLACK);
    display.drawCircleHelper(42,28,6,8,BLACK);
}

void ekranMenu() {
    display.clearDisplay(); durum();
    display.setCursor(0,10); display.println("---ANA MENU---");
    display.setCursor(0,18); display.println("1:Oyun/Hareket");
    display.setCursor(0,26); display.println("2:Gorev Listesi");
    display.setCursor(0,34); display.println("3:Pomodoro");
    display.setCursor(0,42); display.println("4:Cikis");
    display.display();
}

// Görev listesi ekranı
// Jüri ise listede "[Sheets]" etiketi gösterilir
void ekranGorev() {
    display.clearDisplay(); durum();
    display.setCursor(0,9);

    if (juriModu && !gorevListesiHazir) {
        // Jüri girdi ama liste henüz gelmediyse bekle ekranı
        display.println("Liste yukleniyor");
        display.setCursor(0,20); display.println("Lutfen bekleyin...");
        display.display();
        return;
    }

    // Başlık: Jüri modundaysa "SHEETS" yazar
    if (juriModu) {
        display.println("-SHEETS GOREV-");
    } else {
        display.println("-GOREV LISTESI-");
    }

    int bas = seciliIdx - 1;
    if (bas < 0) bas = 0;
    if (bas > TOPLAM - 3) bas = TOPLAM - 3;

    for (int i = 0; i < 3; i++) {
        int idx = bas + i;
        display.setCursor(0, 18 + i * 8);
        display.print(idx == seciliIdx ? ">" : " ");
        display.print(idx == aktifIdx  ? "[X]" : "[ ]");
        String g = gorevler[idx];
        if (g.length() > 9) g = g.substring(0, 9);
        display.print(g);
    }

    display.setCursor(0, 42);
    display.print("3:Sec  4:Menu");
    display.display();
}

void ekranPomodoro() {
    display.clearDisplay(); durum();
    display.setCursor(0,10);
    display.print(durumModu ? "-ODAKLANMA-" : "-MOLA-     ");
    display.setTextSize(2);
    display.setCursor(10,20);
    display.printf("%02d:%02d", kalanDak, kalanSan);
    display.setTextSize(1);
    display.setCursor(0,40);
    display.print(sayacCalis ? "3:Dur 1:+5 2:-5" : "3:Basl 1:+5 2:-5");
    display.display();
}

void ekranOyun() {
    display.clearDisplay(); durum();
    display.setCursor(0,10);
    if (!oyunCalis) {
        display.println("OYUN/HAREKET");
        display.setCursor(0,20); display.println("45dk odak +");
        display.setCursor(0,28); display.println("5dk hareket");
        display.setCursor(0,36); display.println("3:Baslat");
    } else if (durumModu) {
        display.println("-ODAKLANMA-");
        display.setTextSize(2);
        display.setCursor(10,22);
        display.printf("%02d:%02d", oyunDak, oyunSan);
        display.setTextSize(1);
    } else {
        display.println("~HAREKET ET!~");
        display.setCursor(0,20); display.println("Kalk, yuru :)");
        display.setTextSize(2);
        display.setCursor(10,30);
        display.printf("%02d:%02d", oyunDak, oyunSan);
        display.setTextSize(1);
    }
    display.setCursor(0,42); display.print("4:Menu");
    display.display();
}

void ekranSOS() {
    display.clearDisplay();
    display.fillRect(0,0,84,48,BLACK);
    display.setTextColor(WHITE);
    display.setCursor(10,4);  display.println("!!! ACIL !!!");
    display.setCursor(5, 16); display.println("YARDIM SINYALI");
    display.setCursor(18,30); display.println("ATILDI!");
    display.display();
    display.setTextColor(BLACK);
}

void ekranYenile() {
    if (!sistemAcik)        ekranUyku();
    else if (aktifMod == 0) ekranMenu();
    else if (aktifMod == 1) ekranOyun();
    else if (aktifMod == 2) ekranGorev();
    else if (aktifMod == 3) ekranPomodoro();
}

// ============================================================
// SERİ PORTTAN GELEN VERİLERİ İŞLE (Python'dan)
// ============================================================
void serialDinle() {
    if (!Serial.available()) return;
    String veri = Serial.readStringUntil('\n');
    veri.trim();
    if (veri.length() == 0) return;

    // ── Online/Offline durum ──────────────────────────────
    if (veri == "SISTEM_ONLINE")  { wifiBasli = true;  durum(); display.display(); return; }
    if (veri == "SISTEM_OFFLINE") { wifiBasli = false; durum(); display.display(); return; }

    // ── Mail bildirimi ────────────────────────────────────
    if (veri.startsWith("MAIL:")) {
        String icerik = veri.substring(5);
        display.clearDisplay(); durum();
        display.setCursor(0,10); display.println(">> YENI MAIL <<");
        display.setCursor(0,22); display.println(icerik.substring(0,14));
        display.display();
        sesCal("SES_MAIL");
        delay(3000);
        ekranYenile();
        return;
    }

    // ── Python'dan gelen görev listesi ───────────────────
    // Format: GOREV:1.UIUX Tasarim|2.VT Mimarisi|...
    if (veri.startsWith("GOREV:")) {
        String parca = veri.substring(6);
        int idx = 0;
        while (parca.length() > 0 && idx < TOPLAM) {
            int boru = parca.indexOf('|');
            if (boru < 0) {
                gorevler[idx++] = parca;
                break;
            }
            gorevler[idx++] = parca.substring(0, boru);
            parca = parca.substring(boru + 1);
        }
        gorevListesiHazir = true;

        // Eğer şu an görev listesi ekranındaysak, hemen güncelle
        if (aktifMod == 2) ekranGorev();

        Serial.println("DEBUG:Gorev listesi alindi");
        return;
    }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(300);

    SPI.begin(18, 19, 23, 5);
    mfrc522.PCD_Init();

    pinMode(BTN_1,    INPUT_PULLUP);
    pinMode(BTN_2,    INPUT_PULLUP);
    pinMode(BTN_3,    INPUT_PULLUP);
    pinMode(BTN_4,    INPUT_PULLUP);
    pinMode(BTN_ACIL, INPUT_PULLUP);

    display.begin();
    display.setContrast(58);
    display.setTextSize(1);
    display.setTextColor(BLACK);

    // Açılış ekranı
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("DESIGNED BY:");
    display.println("------------");
    display.println(ogrenciAdi);
    display.print("No: ");   display.println(ogrenciNo);
    display.print("Dept: "); display.println(bolum);
    display.display();
    delay(3000);

    // WiFi bağlan
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("WiFi Araniyor");
    display.println(WIFI_SSID);
    display.display();

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int deneme = 0, satir = 0;
    while (WiFi.status() != WL_CONNECTED && deneme < 20) {
        delay(500); deneme++;
        display.print("."); display.display();
        if (++satir > 12) {
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Baglaniyor...");
            satir = 0;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiBasli = true;
        wifiClient.setInsecure();
        configTime(10800, 0, "pool.ntp.org");
        delay(1500);
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("WiFi BAGLANDI!");
        display.println(WiFi.localIP().toString());
        display.display();
        delay(2000);
        Serial.println("SISTEM_ONLINE");
    } else {
        wifiBasli = false;
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("WiFi YOK!");
        display.println("[OFF] MOD");
        display.display();
        delay(2000);
        Serial.println("SISTEM_OFFLINE");
    }

    ekranUyku();
}

// ============================================================
// LOOP
// ============================================================
void loop() {

    serialDinle();

    // ── SOS — her şeyin önünde ────────────────────────────
    if (buton(BTN_ACIL)) {
        ekranSOS();
        sesCal("SES_ACIL");
        if (wifiBasli) {
            String msg = "🚨 ACİL DURUM!\nKullanici: " + aktifKullanici
                       + "\nSaat: " + saatAl();
            bot.sendMessage(CHAT_ID, msg, "");
        }
        delay(3000);
        ekranYenile();
        return;
    }

    // ── SİSTEM KAPALI → RFID BEKLE ───────────────────────
    if (!sistemAcik) {
        static unsigned long saatGnc = 0;
        if (millis() - saatGnc > 10000) {
            saatGnc = millis();
            ekranUyku();
        }
        if (!mfrc522.PICC_IsNewCardPresent() ||
            !mfrc522.PICC_ReadCardSerial()) return;

        String uid = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
            uid += String(mfrc522.uid.uidByte[i], HEX);
        }
        uid.toUpperCase(); uid.trim();

        if (uid == kartAleyna) {
            // ── Aleyna girişi ──────────────────────────────
            aktifKullanici  = "Aleyna";
            juriModu        = false;
            sesCal("SES_HOSGELDIN");
            display.clearDisplay();
            animasyon();
            display.setCursor(2,34); display.println("HOSGELDIN");
            display.setCursor(2,42); display.println("ALEYNA :)");
            display.display(); delay(2500);
            sistemAcik = true; aktifMod = 0;
            ekranMenu();

        } else if (uid == kartJuri) {
            // ── Jüri girişi → Python'a özel sinyal gönder ─
            aktifKullanici  = "Juri Uyesi";
            juriModu        = true;
            gorevListesiHazir = false;  // Yeni liste beklenecek
            sesCal("SES_HOSGELDIN_JURI");  // Python bu sinyalle Sheets çeker
            display.clearDisplay();
            animasyon();
            display.setCursor(2,34); display.println("JURI UYESI");
            display.setCursor(2,42); display.println("HOSGELDINIZ");
            display.display(); delay(2500);
            sistemAcik = true; aktifMod = 0;
            ekranMenu();

        } else {
            sesCal("SES_YETKISIZ");
            display.clearDisplay(); durum();
            display.setCursor(0,12); display.println("YETKISIZ KART!");
            display.setCursor(0,24); display.println("Erisim Reddedildi");
            display.setCursor(0,36); display.print("UID:"); display.println(uid);
            display.display(); delay(3000);
            ekranUyku();
        }

        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }

    // ── MOD 0: ANA MENÜ ──────────────────────────────────
    if (aktifMod == 0) {
        if (buton(BTN_1)) {
            aktifMod = 1;
            oyunDak = 45; oyunSan = 0;
            oyunCalis = false; durumModu = true;
            sesCal("SES_MENU"); ekranOyun();
        }
        else if (buton(BTN_2)) {
            aktifMod = 2;
            seciliIdx = 0;

            if (juriModu && !gorevListesiHazir) {
                // Jüri modundaysa ve liste henüz gelmediyse bekle ekranı
                sesCal("GOREV_GUNCELLE");  // Python'a "hemen çek" komutu
                ekranGorev();              // "Yukleniyor..." ekranı
            } else {
                sesCal("SES_MENU");
                ekranGorev();
            }
        }
        else if (buton(BTN_3)) {
            aktifMod = 3;
            kalanDak = 25; kalanSan = 0;
            durumModu = true; sayacCalis = false;
            sesCal("SES_MENU"); ekranPomodoro();
        }
        else if (buton(BTN_4)) {
            sistemAcik = false;
            juriModu   = false;
            gorevListesiHazir = false;
            aktifKullanici = "";
            oyunCalis  = false;
            sayacCalis = false;
            sesCal("SES_MENU"); ekranUyku();
        }
        return;
    }

    // ── MOD 1: OYUN / HAREKET ────────────────────────────
    if (aktifMod == 1) {
        if (buton(BTN_3)) {
            oyunCalis = !oyunCalis;
            if (oyunCalis) { oyunSayac = millis(); sesCal("SES_BASLA_DERS"); }
            else { sesCal("SES_MENU"); }
            ekranOyun();
        }
        if (oyunCalis && millis() - oyunSayac >= 1000) {
            oyunSayac = millis();
            oyunSan--;
            if (oyunSan < 0) { oyunSan = 59; oyunDak--; }
            if (oyunDak == 0 && oyunSan == 0) {
                durumModu = !durumModu;
                if (!durumModu) { oyunDak = 5;  sesCal("SES_MOLA_BASLADI"); }
                else            { oyunDak = 45; sesCal("SES_DERSE_DON"); }
                oyunSan = 0;
            }
            ekranOyun();
        }
        if (buton(BTN_4)) {
            aktifMod = 0; oyunCalis = false;
            sesCal("SES_MENU"); ekranMenu();
        }
        return;
    }

    // ── MOD 2: GÖREV LİSTESİ ─────────────────────────────
    if (aktifMod == 2) {

        // Jüri modunda liste henüz gelmediyse butonları kilitle
        if (juriModu && !gorevListesiHazir) {
            if (buton(BTN_4)) {
                aktifMod = 0; sesCal("SES_MENU"); ekranMenu();
            }
            return;
        }

        if (buton(BTN_1)) {
            seciliIdx--;
            if (seciliIdx < 0) seciliIdx = TOPLAM - 1;
            sesCal("SES_MENU"); ekranGorev();
        }
        else if (buton(BTN_2)) {
            seciliIdx++;
            if (seciliIdx >= TOPLAM) seciliIdx = 0;
            sesCal("SES_MENU"); ekranGorev();
        }
        else if (buton(BTN_3)) {
            // Görevi seç → Python sesle söyler
            aktifIdx = seciliIdx;
            sesCal("SES_GOREV_" + String(aktifIdx + 1));
            ekranGorev();
        }
        else if (buton(BTN_4)) {
            aktifMod = 0; sesCal("SES_MENU"); ekranMenu();
        }
        return;
    }

    // ── MOD 3: POMODORO ──────────────────────────────────
    if (aktifMod == 3) {
        if (buton(BTN_3)) {
            sayacCalis = !sayacCalis;
            if (sayacCalis) { zamSayac = millis(); sesCal("SES_BASLA_DERS"); }
            else { sesCal("SES_MENU"); }
            ekranPomodoro();
        }
        if (!sayacCalis) {
            if (buton(BTN_1)) {
                kalanDak += 5; if (kalanDak > 90) kalanDak = 90;
                sesCal("SES_MENU"); ekranPomodoro();
            }
            if (buton(BTN_2)) {
                kalanDak -= 5; if (kalanDak < 5) kalanDak = 5;
                sesCal("SES_MENU"); ekranPomodoro();
            }
        }
        if (sayacCalis && millis() - zamSayac >= 1000) {
            zamSayac = millis();
            kalanSan--;
            if (kalanSan < 0) { kalanSan = 59; kalanDak--; }
            if (kalanDak == 0 && kalanSan == 0) {
                durumModu = !durumModu;
                if (!durumModu) { kalanDak = 5;  sesCal("SES_MOLA_BASLADI"); }
                else            { kalanDak = 25; sesCal("SES_DERSE_DON"); }
                kalanSan = 0;
            }
            ekranPomodoro();
        }
        if (buton(BTN_4)) {
            aktifMod = 0; sayacCalis = false;
            sesCal("SES_MENU"); ekranMenu();
        }
        return;
    }
}
