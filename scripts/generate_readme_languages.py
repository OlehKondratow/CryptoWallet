#!/usr/bin/env python3
"""
Generate README in multiple languages (EN, RU, PL) from base English README.
Translates section headers and descriptive text while preserving links and code.

Usage:
    python3 scripts/generate_readme_languages.py
    
Output:
    - README.md (English - unchanged)
    - README_ru.md (Russian)
    - README_pl.md (Polish)
"""

import os
import re
from pathlib import Path

# Translation dictionaries - organized by language and priority (longest phrases first)
TRANSLATIONS = {
    'ru': [
        # Full sentences (highest priority)
        ('A microcontroller application for secure Bitcoin transaction signing on STM32H743. Integrates trezor-crypto, FreeRTOS, LwIP, SSD1306 UI, and WebUSB.',
         'Приложение микроконтроллера для безопасного подписания Bitcoin-транзакций на STM32H743. Интегрирует trezor-crypto, FreeRTOS, LwIP, SSD1306 UI и WebUSB.'),
        
        # Section descriptions
        ('Core signing logic, key management, validation, and cryptographic operations.',
         'Логика подписания, управление ключами, валидация и криптографические операции.'),
        ('Network stack (LwIP/Ethernet), USB (WebUSB), time synchronization.',
         'Сетевой стек (LwIP/Ethernet), USB (WebUSB), синхронизация времени.'),
        ('Display management, button handling, system events, and indicators.',
         'Управление дисплеем, обработка кнопок, системные события и индикаторы.'),
        ('HAL initialization, clocking, interrupt handlers, driver configuration.',
         'Инициализация HAL, тактирование, обработчики прерываний, конфигурация драйверов.'),
        
        # Headers and section titles
        ('## 🔐 Core & Security', '## 🔐 Ядро и безопасность'),
        ('## 📡 Communication Interfaces', '## 📡 Интерфейсы связи'),
        ('## 🎨 User Experience', '## 🎨 Пользовательский опыт'),
        ('## ⚙️ System & Hardware', '## ⚙️ Система и оборудование'),
        ('## 📚 Documentation and Reference', '## 📚 Документация и справочные материалы'),
        ('## 🚀 Quick Start', '## 🚀 Быстрый старт'),
        ('## 📋 Complete Module Index', '## 📋 Полный индекс модулей'),
        
        # Subsection headers
        ('### Documentation Structure', '### Структура документации'),
        ('### How to Read a Module', '### Как читать модуль'),
        ('### Main Commands', '### Основные команды'),
        
        # Common phrases
        ('Documentation available in:', 'Документация доступна на:'),
        ('Full documentation:', 'Полная документация:'),
        ('Headers:', 'Заголовки:'),
        ('(this file)', '(этот файл)'),
        ('**Full documentation:**', '**Полная документация:**'),
        ('**Headers:**', '**Заголовки:**'),
        
        # Detailed descriptions
        ('1. **Code (.c/.h)** — minimal @brief/@details, API level',
         '1. **Код (.c/.h)** — минимальные @brief/@details, уровень API'),
        ('2. **documentation/*.md** — numbered manual (trust model → build/CI); see `documentation/README.md`',
         '2. **documentation/*.md** — единый мануал (модель доверия → сборка/CI); см. `documentation/README.md`'),
        ('3. **Doxygen HTML** — code cross-references (`make docs-doxygen`)',
         '3. **Doxygen HTML** — перекрёстные ссылки кода (`make docs-doxygen`)'),
        
        ('1. Open [documentation/README.md](documentation/README.md)',
         '1. Откройте [documentation/README.md](documentation/README.md)'),
        ('2. Find your module of interest',
         '2. Найдите интересующий вас модуль'),
        ('3. Start with **Abstract** (business logic) → **Logic Flow** (algorithm) → **Dependencies**',
         '3. Начните с **Abstract** (бизнес-логика) → **Logic Flow** (алгоритм) → **Dependencies**'),
        ('4. Follow **Relations** for broader context',
         '4. Следите за **Relations** для более широкого контекста'),
        
        # Commands
        ('make docs-doxygen    # Generate Doxygen',
         'make docs-doxygen    # Генерировать Doxygen'),
        ('make build          # Build',
         'make build          # Сборка'),
        ('make minimal-lwip   # Minimal build',
         'make minimal-lwip   # Минимальная сборка'),
        ('make flash          # Flash to STM32',
         'make flash          # Загрузить на STM32'),
    ],
    'pl': [
        # Full sentences (highest priority)
        ('A microcontroller application for secure Bitcoin transaction signing on STM32H743. Integrates trezor-crypto, FreeRTOS, LwIP, SSD1306 UI, and WebUSB.',
         'Aplikacja mikrokontrolera do bezpiecznego podpisywania transakcji Bitcoin na STM32H743. Integruje trezor-crypto, FreeRTOS, LwIP, SSD1306 UI i WebUSB.'),
        
        # Section descriptions
        ('Core signing logic, key management, validation, and cryptographic operations.',
         'Logika podpisywania, zarządzanie kluczami, walidacja i operacje kryptograficzne.'),
        ('Network stack (LwIP/Ethernet), USB (WebUSB), time synchronization.',
         'Stos sieciowy (LwIP/Ethernet), USB (WebUSB), synchronizacja czasu.'),
        ('Display management, button handling, system events, and indicators.',
         'Zarządzanie wyświetlaczem, obsługa przycisków, zdarzenia systemowe i wskaźniki.'),
        ('HAL initialization, clocking, interrupt handlers, driver configuration.',
         'Inicjalizacja HAL, taktowanie, procedury obsługi przerwań, konfiguracja sterownika.'),
        
        # Headers and section titles
        ('## 🔐 Core & Security', '## 🔐 Jądro i bezpieczeństwo'),
        ('## 📡 Communication Interfaces', '## 📡 Interfejsy komunikacyjne'),
        ('## 🎨 User Experience', '## 🎨 Doświadczenie użytkownika'),
        ('## ⚙️ System & Hardware', '## ⚙️ System i sprzęt'),
        ('## 📚 Documentation and Reference', '## 📚 Dokumentacja i materiały referencyjne'),
        ('## 🚀 Quick Start', '## 🚀 Szybki start'),
        ('## 📋 Complete Module Index', '## 📋 Pełny indeks modułów'),
        
        # Subsection headers
        ('### Documentation Structure', '### Struktura dokumentacji'),
        ('### How to Read a Module', '### Jak czytać moduł'),
        ('### Main Commands', '### Główne polecenia'),
        
        # Common phrases
        ('Documentation available in:', 'Dokumentacja dostępna w:'),
        ('Full documentation:', 'Pełna dokumentacja:'),
        ('Headers:', 'Nagłówki:'),
        ('(this file)', '(ten plik)'),
        ('**Full documentation:**', '**Pełna dokumentacja:**'),
        ('**Headers:**', '**Nagłówki:**'),
        
        # Detailed descriptions
        ('1. **Code (.c/.h)** — minimal @brief/@details, API level',
         '1. **Kod (.c/.h)** — minimalne @brief/@details, poziom API'),
        ('2. **documentation/*.md** — numbered manual (trust model → build/CI); see `documentation/README.md`',
         '2. **documentation/*.md** — jeden podręcznik (model zaufania → build/CI); zob. `documentation/README.md`'),
        ('3. **Doxygen HTML** — code cross-references (`make docs-doxygen`)',
         '3. **Doxygen HTML** — odsyłacze krzyżowe kodu (`make docs-doxygen`)'),
        
        ('1. Open [documentation/README.md](documentation/README.md)',
         '1. Otwórz [documentation/README.md](documentation/README.md)'),
        ('2. Find your module of interest',
         '2. Znajdź interesujący Cię moduł'),
        ('3. Start with **Abstract** (business logic) → **Logic Flow** (algorithm) → **Dependencies**',
         '3. Zacznij od **Abstract** (logika biznesowa) → **Logic Flow** (algorytm) → **Dependencies**'),
        ('4. Follow **Relations** for broader context',
         '4. Śledź **Relations** dla szerszego kontekstu'),
        
        # Commands
        ('make docs-doxygen    # Generate Doxygen',
         'make docs-doxygen    # Generuj Doxygen'),
        ('make build          # Build',
         'make build          # Zbuduj'),
        ('make minimal-lwip   # Minimal build',
         'make minimal-lwip   # Minimalna kompilacja'),
        ('make flash          # Flash to STM32',
         'make flash          # Wgrać do STM32'),
    ]
}

def translate_readme(readme_path, lang):
    """Translate README to specified language."""
    with open(readme_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if lang == 'en':
        return content
    
    # Protect code blocks and links before translation
    code_blocks = []
    links = []
    
    # Extract code blocks
    def extract_code(match):
        code_blocks.append(match.group(0))
        return f"<<<CODE_BLOCK_{len(code_blocks)-1}>>>"
    
    content = re.sub(r'```[^`]*```', extract_code, content, flags=re.DOTALL)
    
    # Extract markdown links to preserve them
    def extract_link(match):
        links.append(match.group(0))
        return f"<<<LINK_{len(links)-1}>>>"
    
    content = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', extract_link, content)
    
    # Apply translations
    translations = TRANSLATIONS.get(lang, [])
    for original, translated in translations:
        content = content.replace(original, translated)
    
    # Restore links
    for i, link in enumerate(links):
        content = content.replace(f"<<<LINK_{i}>>>", link)
    
    # Restore code blocks
    for i, code in enumerate(code_blocks):
        content = content.replace(f"<<<CODE_BLOCK_{i}>>>", code)
    
    # Update language links in header
    # First, remove any "(this file)" or translated variants from all links
    content = re.sub(r'(- 🇬🇧 \[English\]\(README\.md\))\s+\([^)]+\)', r'\1', content)
    content = re.sub(r'(- 🇷🇺 \[Русский\]\(README_ru\.md\))\s+\([^)]+\)', r'\1', content)
    content = re.sub(r'(- 🇵🇱 \[Polski\]\(README_pl\.md\))\s+\([^)]+\)', r'\1', content)
    
    # Then add the appropriate "(this file)" indicator for current language
    if lang == 'ru':
        content = re.sub(
            r'- 🇷🇺 \[Русский\]\(README_ru\.md\)',
            r'- 🇷🇺 [Русский](README_ru.md) (этот файл)',
            content
        )
    elif lang == 'pl':
        content = re.sub(
            r'- 🇵🇱 \[Polski\]\(README_pl\.md\)',
            r'- 🇵🇱 [Polski](README_pl.md) (ten plik)',
            content
        )
    
    return content

def main():
    readme_path = Path('README.md')
    
    if not readme_path.exists():
        print(f"Error: {readme_path} not found")
        return
    
    print("Generating README in multiple languages...")
    
    # Generate for all languages
    for lang, lang_name in [('en', 'English'), ('ru', 'Russian'), ('pl', 'Polish')]:
        translated = translate_readme(readme_path, lang)
        
        if lang == 'en':
            output_file = 'README.md'
        else:
            output_file = f'README_{lang}.md'
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(translated)
        
        print(f"✅ Generated {output_file} ({lang_name})")
    
    print("\n✅ All README files generated successfully!")

if __name__ == '__main__':
    main()
