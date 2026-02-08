# Xiao ESP32-S3 Sense Camera - Aktualny stan

To repozytorium zawiera teraz minimalne firmware dla ESP32-S3, które:
- łączy się z Wi-Fi,
- uruchamia serwer HTTP,
- udostępnia podgląd MJPEG pod `/stream`,
- udostępnia prosty interfejs WWW pod `/` z osadzonym podglądem.

Obsługa karty SD i nagrywania została usunięta.

## Plik firmware
- `xiaosensespycam.ino`

## Aktualne URL-e (gdy urządzenie jest w LAN)
- UI: `http://<ip-urzadzenia>/`
- Stream (MJPEG): `http://<ip-urzadzenia>/stream`

## Ustalenia
- Zostajemy przy JPEG/MJPEG (brak kodowania wideo na urządzeniu).
- Strumień ma działać w aplikacji mobilnej bez konfiguracji przez użytkownika.
- Plan to użyć chmury jako pośrednika (backend w Node.js).
- Na ten moment brak autoryzacji (faza testowa).

## Proponowana architektura (kolejny krok)
1. ESP32 wysyła klatki JPEG do publicznego backendu (HTTP POST lub WebSocket).
2. Backend przechowuje ostatnią klatkę w pamięci.
3. Backend wystawia `/mjpeg` po HTTPS dla aplikacji Flutter.
4. Aplikacja Flutter wyświetla podgląd z backendu.

## Hosting backendu (najprostsza opcja)
- Render Web Service (publiczny adres `onrender.com`).
- Aplikacja Node.js nasłuchuje na `0.0.0.0` i `process.env.PORT`.

## Następne kroki (jeszcze nie zrobione)
- Dodać backend Node.js do tego repozytorium:
  - endpoint do przyjmowania klatek JPEG z urządzenia,
  - endpoint `/mjpeg` dla aplikacji.
- Zmienić `xiaosensespycam.ino`, aby wysyłał klatki JPEG do backendu.
- Dodać widget Flutter do podglądu MJPEG.

