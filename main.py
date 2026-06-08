# ============================================================
# MAYA PYTHON — CORE CONTROL SCRIPT
# Integration: Google Sheets API, Gmail API, Telegram Bot, 
#              and Serial Hardware Communication
# ============================================================

import os, time, threading
import serial
import numpy as np
import pygame
import requests
from gtts import gTTS
from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build

# -- CONFIGURATION PARAMETERS ----------------------------------
SERIAL_PORT    = 'COM8'
BAUD_RATE      = 115200
SAMPLE_RATE    = 44100
SPREADSHEET_ID = '13YFgAqsVRvh2TMV2JGWHa-rwHozTg4TEq6V6egfVvOA'
RANGE_NAME     = 'Sayfa1!C3:C14'
BOT_TOKEN      = '8516599187:AAE9pUeVJ26EsVQpkpMIUqrXOWg_aWYSZPs'
CHAT_ID        = '8589249980'
SCOPES = [
    'https://www.googleapis.com/auth/spreadsheets.readonly',
    'https://www.googleapis.com/auth/gmail.readonly'
]

# -- GLOBAL STATE VARIABLES ------------------------------------
ses_kilidi    = threading.Lock()
son_mail_id   = None
gorev_listesi = []     
juri_aktif    = False  

# -- AUDIO ENGINE & TTS CONTROL --------------------------------
pygame.mixer.init(frequency=SAMPLE_RATE, size=-16, channels=2, buffer=512)

def ton(frekans, sure_ms):
    if frekans <= 0:
        time.sleep(sure_ms / 1000.0)
        return
    sure_s = sure_ms / 1000.0
    t      = np.linspace(0, sure_s, int(SAMPLE_RATE * sure_s), False)
    dalga = (np.sin(2 * np.pi * frekans * t) * 32767 * 0.5).astype(np.int16)
    stereo = np.column_stack((dalga, dalga))  
    pygame.sndarray.make_sound(stereo).play()
    time.sleep(sure_s)

def konuş(metin):
    with ses_kilidi:
        try:
            print(f"[SES] {metin}")
            tts   = gTTS(text=metin, lang='tr', slow=False)
            dosya = f"maya_{threading.get_ident()}.mp3"
            tts.save(dosya)
            pygame.mixer.music.load(dosya)
            pygame.mixer.music.play()
            while pygame.mixer.music.get_busy():
                time.sleep(0.1)
            pygame.mixer.music.unload()
            if os.path.exists(dosya):
                os.remove(dosya)
        except Exception as e:
            print(f"⚠️ Ses hatası: {e}")

# -- AUDIO AUDIO MELODIES --------------------------------------
def melodi_hosgeldin():
    ton(523,100); time.sleep(0.05)
    ton(659,100); time.sleep(0.05)
    ton(784,100); time.sleep(0.05)
    ton(1047,200)

def melodi_basla():
    ton(523,90); time.sleep(0.05)
    ton(659,90); time.sleep(0.05)
    ton(784,150)

def melodi_mola():
    ton(784,150); time.sleep(0.1)
    ton(659,150); time.sleep(0.1)
    ton(523,300)

def melodi_acil():
    for _ in range(4):
        ton(1400,150); time.sleep(0.05)
        ton(750, 150); time.sleep(0.05)

def melodi_menu():     ton(440, 60)
def melodi_yetkisiz(): ton(220,200); time.sleep(0.1); ton(180,300)
def melodi_mail():     ton(880,100); time.sleep(0.05); ton(1100,100)

# -- TELEGRAM NOTIFICATION SERVICE -----------------------------
def telegram_gonder(mesaj):
    try:
        requests.post(
            f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage",
            json={"chat_id": CHAT_ID, "text": mesaj},
            timeout=5
        )
        print("📲 Telegram gönderildi.")
    except Exception as e:
        print(f"⚠️ Telegram hatası: {e}")

# -- GOOGLE OAUTH2 AUTHENTICATION ------------------------------
def google_baglan():
    creds = None
    if os.path.exists('token.json'):
        creds = Credentials.from_authorized_user_file('token.json', SCOPES)
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            try:
                creds.refresh(Request())
            except Exception:
                creds = None
        if not creds:
            if not os.path.exists('credentials.json'):
                print("🚨 credentials.json bulunamadı!")
                return None
            flow  = InstalledAppFlow.from_client_secrets_file('credentials.json', SCOPES)
            creds = flow.run_local_server(port=0)
        with open('token.json', 'w') as f:
            f.write(creds.to_json())
    return creds

# -- GOOGLE SHEETS DATA FETCHING -------------------------------
def sheets_cek(creds):
    try:
        service = build('sheets', 'v4', credentials=creds, cache_discovery=False)
        result  = service.spreadsheets().values().get(
            spreadsheetId=SPREADSHEET_ID, range=RANGE_NAME).execute()
        rows  = result.get('values', [])
        liste = []
        for i in range(12):
            if i < len(rows) and rows[i]:
                metin = rows[i][0].strip()
                metin = (metin[:11] + "...") if len(metin) > 14 else metin.ljust(14)
            else:
                metin = f"{i+1}.Bos Gorev  "
            liste.append(metin)
        print(f"✅ Sheets'ten {len(liste)} görev çekildi:")
        for i, g in enumerate(liste):
            print(f"   {i+1}. {g.strip()}")
        return liste
    except Exception as e:
        print(f"📊 Sheets hatası: {e}")
        return None

def gorev_gonder(esp32, liste):
    if not liste:
        return
    try:
        payload = "GOREV:" + "|".join(liste) + "\n"
        esp32.write(payload.encode('utf-8'))
        print("📤 Görev listesi ESP32'ye gönderildi.")
    except Exception as e:
        print(f"⚠️ Görev gönderme hatası: {e}")

# -- GOOGLE GMAIL PROCESSING SERVICE ---------------------------
def gmail_kontrol(creds):
    global son_mail_id
    try:
        service  = build('gmail', 'v1', credentials=creds, cache_discovery=False)
        results  = service.users().messages().list(
            userId='me', q='is:unread label:INBOX', maxResults=1).execute()
        messages = results.get('messages', [])
        if not messages:
            return None
        mid = messages[0]['id']
        if mid == son_mail_id:
            return None
        son_mail_id = mid
        msg     = service.users().messages().get(
            userId='me', id=mid, format='metadata',
            metadataHeaders=['From','Subject']).execute()
        headers = msg.get('payload', {}).get('headers', [])
        gonderen, konu = "Bilinmeyen", "Konu Yok"
        for h in headers:
            if h['name'] == 'From':
                gonderen = h['value'].split('<')[0].strip()
            if h['name'] == 'Subject':
                konu = h['value'].strip()
        tablo = str.maketrans("çğıöşüÇĞİÖŞÜ", "cgiosuCGIOSU")
        return f"{gonderen.translate(tablo)[:14]}-{konu.translate(tablo)[:14]}"
    except Exception as e:
        print(f"📩 Gmail hatası: {e}")
        return None

# -- SERIAL SIGNAL INTERFACE -----------------------------------
def sinyal_isle(sinyal, esp32, creds):
    global gorev_listesi, juri_aktif

    print(f"[ESP32] {sinyal}")

    if sinyal == "SES_HOSGELDIN":
        juri_aktif = False
        melodi_hosgeldin()
        konuş("Hoş geldiniz Aleyna. Kimlik doğrulama başarılı.")

    elif sinyal == "SES_HOSGELDIN_JURI":
        juri_aktif = True
        melodi_hosgeldin()
        konuş("Hoş geldiniz sayın jüri üyesi.")
        print("👔 Jüri girişi → Sheets çekiliyor...")
        yeni = sheets_cek(creds)
        if yeni:
            gorev_listesi = yeni
            gorev_gonder(esp32, gorev_listesi)
            konuş("Görev listesi yüklendi.")
        else:
            konuş("Görev listesi alınamadı, internet bağlantısını kontrol edin.")

    elif sinyal == "SES_YETKISIZ":
        juri_aktif = False
        melodi_yetkisiz()
        konuş("Yetkisiz kart. Erişim engellendi.")

    elif sinyal == "GOREV_GUNCELLE":
        if juri_aktif:
            print("🔄 Jüri güncelleme isteği → Sheets çekiliyor...")
            yeni = sheets_cek(creds)
            if yeni:
                gorev_listesi = yeni
                gorev_gonder(esp32, gorev_listesi)
                konuş("Görevler güncellendi.")
            else:
                konuş("Bağlantı hatası. Görevler güncellenemedi.")
        else:
            print("ℹ️ Güncelleme isteği geldi ama jüri aktif değil.")

    elif sinyal == "SES_BASLA_DERS":
        melodi_basla()
        konuş("Odaklanma süresi başladı. İyi çalışmalar.")

    elif sinyal == "SES_MOLA_BASLADI":
        melodi_mola()
        konuş("Tebrikler, mola vakti. Biraz dinlen.")

    elif sinyal == "SES_DERSE_DON":
        melodi_basla()
        konuş("Mola bitti. Hadi derse dön!")

    elif sinyal == "SES_MENU":
        melodi_menu()

    elif sinyal == "SES_MAIL":
        if juri_aktif:
            melodi_mail()

    elif sinyal in ("SES_ACIL", "KOMUT:SOS_TETIKLENDI"):
        melodi_acil()
        threading.Thread(
            target=telegram_gonder,
            args=("🚨 ACİL DURUM! MAYA SOS butonuna basıldı!",),
            daemon=True
        ).start()
        konuş("Acil durum! Yardım sinyali Telegram üzerinden gönderildi.")

    elif sinyal.startswith("SES_GOREV_"):
        try:
            idx = int(sinyal.split("_")[2]) - 1  
            print(f"   Görev indeks: {idx}, Liste uzunluğu: {len(gorev_listesi)}")

            if gorev_listesi and 0 <= idx < len(gorev_listesi):
                ad = gorev_listesi[idx].strip()
                print(f"   Seçilen görev: '{ad}'")
                melodi_menu()
                konuş(f"{ad} görevi seçildi ve aktif olarak işaretlendi.")
            else:
                print("⚠️ Görev listesi boş veya indeks hatalı!")
                if juri_aktif and creds:
                    print("   Sheets'ten yeniden çekiliyor...")
                    yeni = sheets_cek(creds)
                    if yeni:
                        gorev_listesi = yeni
                        gorev_gonder(esp32, gorev_listesi)
                        if 0 <= idx < len(gorev_listesi):
                            ad = gorev_listesi[idx].strip()
                            konuş(f"{ad} görevi seçildi.")
                else:
                    konuş("Görev listesi henüz yüklenmedi.")
        except Exception as e:
            print(f"⚠️ Görev ses hatası: {e}")

# -- MAIN RUNTIME EXECUTION ------------------------------------
def main():
    global gorev_listesi, juri_aktif

    print("=" * 54)
    print("     MAYA PYTHON — CORE SYSTEM SYSTEM")
    print("=" * 54)

    print("🔑 Google API bağlanıyor...")
    creds = google_baglan()
    print("✅ Google API hazır!" if creds else "⚠️  Google API yok, ses modu aktif.")

    print(f"\n🔌 ESP32 bağlanıyor ({SERIAL_PORT})...")
    try:
        esp32 = serial.Serial(port=SERIAL_PORT, baudrate=BAUD_RATE, timeout=0.1)
        time.sleep(2)
        esp32.write(b"SISTEM_ONLINE\n")
        print("✅ ESP32 bağlandı!\n")
        print("ℹ️  Sistem hazır. Kart okutun...\n")
    except Exception as e:
        print(f"🚨 ESP32 bağlantı hatası: {e}")
        return

    son_gmail = 0
    GMAIL_ARALIK = 6  

    while True:
        try:
            if esp32.in_waiting > 0:
                satir = esp32.readline().decode('utf-8', errors='ignore').strip()
                if satir:
                    threading.Thread(
                        target=sinyal_isle,
                        args=(satir, esp32, creds),
                        daemon=True
                    ).start()

            su_an = time.time()

            if juri_aktif and creds and (su_an - son_gmail > GMAIL_ARALIK):
                son_gmail = su_an
                yeni_mail = gmail_kontrol(creds)
                if yeni_mail:
                    print(f"📩 Yeni mail: {yeni_mail}")
                    esp32.write(f"MAIL:{yeni_mail}\n".encode('utf-8'))
                    kim = yeni_mail.split("-")[0]
                    threading.Thread(
                        target=konuş,
                        args=(f"{kim} tarafından yeni bir e-posta aldınız.",),
                        daemon=True
                    ).start()

            time.sleep(0.01)

        except KeyboardInterrupt:
            print("\n🛑 MAYA kapatılıyor...")
            try:
                esp32.write(b"SISTEM_OFFLINE\n")
            except:
                pass
            break
        except Exception as e:
            print(f"⚠️ Hata: {e}")
            time.sleep(1)

    pygame.mixer.quit()
    print("Sistem kapatıldı. İyi çalışmalar!")

if __name__ == '__main__':
    main()
