# MAYA-Akilli-Asistan
BÖZ214 Fiziksel Programlama dersi kapsamında geliştirilen MAYA, ESP32 ve Python tabanlı hibrit bir akıllı görev ve asistan sistemidir. Sistem; RFID tabanlı yetkilendirme, Google Sheets entegrasyonu, gerçek zamanlı görev yönetimi, sesli geri bildirim, RTC destekli zaman takibi ve acil durum bildirim özellikleri sunmaktadır.
# MAYA: Hibrit Akıllı Görev ve Asistan Sistemi

BÖZ214 Fiziksel Programlama dersi kapsamında geliştirilen MAYA, ESP32 ve Python tabanlı hibrit bir akıllı görev ve asistan sistemidir. Sistem; RFID tabanlı yetkilendirme, Google Sheets entegrasyonu, gerçek zamanlı görev yönetimi, sesli geri bildirim (TTS), zaman yönetimi ve acil durum bildirim özellikleri sunmaktadır.

---

## 🛠️ Donanım Bileşenleri & Pin Mimarisi
Proje kapsamında kullanılan donanım bileşenleri ve ESP32 mikrodenetleyici üzerindeki pin eşleşmeleri aşağıda belirtilmiştir:

* **Mikrodenetleyici:** ESP32 NodeMCU
* **Ekran (Ekran):** Nokia 5110 LCD Ekran (`DC: 4`, `CE: 16`, `RST: 17`)
* **Haberleşme (SPI):** `SCK: 18`, `MISO: 19`, `MOSI: 23`, `SS/SDA: 5`
* **Kart Okuyucu:** MFRC522 RFID Modülü (`RST: 27`)
* **Kullanıcı Girişleri (Butonlar):** * `BTN_1 (Yukarı): 22`
    * `BTN_2 (Aşağı): 21`
    * `BTN_3 (Seç / Başlat): 14`
    * `BTN_4 (Menü / Çıkış): 13`
    * `BTN_ACIL (SOS): 32`

---

## 🚀 Öne Çıkan Özellikler
* **Çift Faktörlü Yetkilendirme:** RFID kart okuma algoritması ile kullanıcı bazlı dinamik arayüz (Standart Kullanıcı / Jüri Üyesi Modu).
* **Bulut Entegrasyonu:** Google Sheets API üzerinden gerçek zamanlı görev listesi senkronizasyonu.
* **Akıllı Bildirim Sistemi:** Gmail API ile senkronize çalışan arka plan e-posta takibi ve anlık Telegram SOS entegrasyonu.
* **Zaman Yönetimi:** Yerleşik Pomodoro sayacı ve 45+5 Aktif Hareket takip algoritmaları.
* **Ses sentezi (TTS):** Python gTTS (Google Text-to-Speech) kütüphanesi ile tamamen Türkçe sesli asistan geri bildirimi.

---

## 💻 Yazılım Kurulumu & Bağımlılıklar

### 1. ESP32 (Firmware)
`maya_esp32.ino` dosyasını Arduino IDE ile açarak aşağıdaki kütüphaneleri yükleyiniz:
* `SPI.h` & `WiFi.h`
* `UniversalTelegramBot`
* `MFRC522`
* `Adafruit_GFX` & `Adafruit_PCD8544`

### 2. Python (Arka Plan Servisi)
Gerekli Python kütüphanelerini yüklemek için terminalde aşağıdaki komutu çalıştırın:
```bash
pip install pyserial numpy pygame requests gTTS google-auth google-auth-oauthlib google-api-python-client


Güvenlik ve Kimlik Doğrulama Notu
 ÖNEMLİ SİBER GÜVENLİK NOTU: Google API kimlik doğrulama sertifikaları (credentials.json ve token.json) veri güvenliği standartları gereği ve hassas anahtarlar içermesi sebebiyle bu açık kaynaklı depoya dahil edilmemiştir.

Sistemi yerel ortamda test etmek isteyen jüri ve geliştiricilerin, Google Cloud Console üzerinden kendi OAuth 2.0 İstemci Kimliklerini oluşturarak proje ana dizinine eklemeleri gerekmektedir. Örnek şablon yapısı credentials.json.example dosyasında sunulmuştur.

 Geliştirici Bilgileri
Proje Geliştiricisi: Aleyna Nur Erdoğan

Bölüm: Bilgisayar ve Öğretim Teknolojileri Eğitimi Bölümü (BÖTE)

Ders: BÖZ214 Fiziksel Programlama

Öğretim Elemanı: Mehmet TEKEREK
