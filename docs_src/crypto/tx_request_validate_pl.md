\page tx_request_validate "tx_request_validate: bramka walidacji do podpisu"
\related tx_request_validate
\related tx_recipient_format_ok
\related tx_amount_format_ok
\related tx_currency_supported

# `tx_request_validate.c` + `tx_request_validate.h`

<brief>Moduł `tx_request_validate` jest bramką ochronną przed podpisywaniem: sprawdza pola dostarczone przez hosta (adres odbiorcy, kwotę, walutę) pod kątem podstawowej zgodności formatu, pomagając uniknąć wyraźnie błędnych danych na SSD1306/w dziennikach i chroniąc potok podpisywania przed śmietnymi danymi wejściowymi.</brief>

## Przegląd

Moduł `tx_request_validate` jest bramką ochronną przed podpisywaniem: sprawdza pola dostarczone przez hosta (adres odbiorcy, kwotę, walutę) pod kątem podstawowej zgodności formatu, pomagając uniknąć wyraźnie błędnych danych na SSD1306/w dziennikach i chroniąc potok podpisywania przed śmietnymi danymi wejściowymi.

## Przepływ logiki (validation gates)

Funkcja `tx_request_validate()` wykonuje sekwencyjne kontrole:
1. Null-check na sam obiekt
2. **Recipient:** nie pusty, długość < `TX_RECIPIENT_LEN`, format OK (Base58 dla P2PKH/P2SH lub bech32 dla SegWit)
3. **Amount:** nie pusty, jest ciąg dziesiętny, zawiera co najmniej jedną cyfrę, max jeden punkt dziesiętny, `atof` > 0
4. **Currency:** jeśli nie pusty, sprawdź względem whitelist (BTC, ETH, LTC, BCH, DOGE, DASH, XMR); jeśli pusty, domyślnie BTC
5. Na pierwszym błędzie zwróć odpowiadający enum `TX_VALID_ERR_*`

Każda kontrola ma swoją funkcję pomocniczą (`tx_recipient_format_ok`, `tx_amount_format_ok`, `tx_currency_supported`).

## Przerwania i rejestry

Brak ISR/rejestrów: moduł to czista logika walidacji ciągów.

## Czasy i warunki rozgałęzienia

| Kontrola | Warunek niepowodzenia | Kod błędu |
|----------|---|---|
| recipient pusty | `recipient[0] == '\0'` | `TX_VALID_ERR_RECIPIENT_EMPTY` |
| recipient za długo | `len >= TX_RECIPIENT_LEN` | `TX_VALID_ERR_RECIPIENT_TOO_LONG` |
| recipient zły format | nie Base58 (dla '1'/'3') lub nie bech32 (dla "bc1") | `TX_VALID_ERR_RECIPIENT_INVALID_CHARS` |
| amount pusty | `amount[0] == '\0'` | `TX_VALID_ERR_AMOUNT_EMPTY` |
| amount nie dziesiętny | zawiera znaki non-digit/non-dot | `TX_VALID_ERR_AMOUNT_INVALID_CHARS` |
| amount zero/ujemny | `atof(amount) <= 0` | `TX_VALID_ERR_AMOUNT_ZERO` |
| currency nieobsługiwana | nie na whitelist i nie pusta | `TX_VALID_ERR_CURRENCY_UNSUPPORTED` |

## Zależności

Bezpośrednie:
- Input: `wallet_tx_t` z polami recipient/amount/currency
- Output: `tx_validate_result_str()` dla czytelnych dla człowieka wiadomości o błędach
- Brak zależności od zewnętrznej kryptografii lub sprzętu

Pośrednie:
- `task_net.md` (wywołuje walidację na otrzymanym żądaniu HTTP)
- `task_sign.md` (otrzymuje zwalidowaną transakcję z kolejki)

## Relacje modułów

- `task_net.md` (żąda walidacji przed umieszczeniem w kolejce)
- `task_sign.md` (konsumuje zwalidowane transakcje)
- `wallet_shared.md` (kontrakt danych transakcji)
