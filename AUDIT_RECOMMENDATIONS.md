# Language Compliance Audit - Recommendations

**Audit Date:** 2026-03-19  
**Compliance Status:** 🔴 CRITICAL (6% complete)

---

## Executive Recommendation

### Immediate Action Required

The `docs_src/` directory has a **critical language mismatch**: 30 modules are still in Russian when they should be in English. This blocks Phase 2 completion and affects documentation usability.

**Recommendation:** Begin Tier 1 translations immediately to restore compliance.

---

## Decision Framework

### Option A: Manual Translation (RECOMMENDED)
**Quality-Focused Approach**

```
Workflow:
  1. Read Russian .md file
  2. Manually translate to English
  3. Save as *.md (English base)
  4. Save original as *_ru.md (Russian version)
  5. Translate to Polish → *_pl.md
  6. Verify with compliance checker
  7. Commit

Pros:
  ✅ High quality technical translations
  ✅ Preserves domain-specific terminology
  ✅ Maintains Doxygen directives perfectly
  ✅ Better for educational documentation
  
Cons:
  ❌ Slower (24-31 hours total)
  ❌ Requires careful attention to technical terms
  
Recommended for: Tier 1 (5 critical modules)
Timeline: 4-6 hours
Result: ~25% compliance

Effort: One focused work session (or split into 2-3 sessions)
```

### Option B: AI-Assisted Translation
**Speed-Optimized Approach**

```
Workflow:
  1. Use Claude/ChatGPT for initial translation
  2. Manual review of technical terms
  3. Fix any terminology issues
  4. Save versions
  5. Spot-check with compliance tool
  6. Commit

Pros:
  ✅ Faster (12-15 hours total)
  ✅ Less manual effort
  ✅ Still maintains quality for technical content
  
Cons:
  ❌ May need terminology corrections
  ❌ Requires careful review
  
Recommended for: Tier 2+3 (if doing all at once)
Timeline: 12-15 hours
Result: 100% compliance
```

### Option C: Automated + Manual Review
**Balanced Approach**

```
Workflow:
  1. Create automation script:
     - Rename *.md → *.temp
     - Create *_ru.md from .temp
     - Use Google Translate API on .temp → new *.md
  2. Manual review of English output
  3. Fix terminology
  4. Create Polish manually or via API
  5. Verify
  6. Commit

Pros:
  ✅ Fast automation for bulk work
  ✅ Maintains quality through review
  ✅ Can be reused for future docs
  
Cons:
  ❌ Requires API setup (Google Cloud)
  ❌ Translation quality varies by topic
  
Recommended for: Tier 2+3 (bulk operations)
Timeline: 15-20 hours
Result: 100% compliance
```

---

## Recommended Hybrid Approach

### Phase 1: Quick Start (Tier 1 - Today)
**Option A: Manual Translation (5 modules)**

```
Priority Order:
1. task_sign.md (82 lines) - Most critical, defines signing FSM
2. crypto_wallet.md (71 lines) - Core cryptography
3. task_net.md (78 lines) - Network handling
4. wallet_shared.md (61 lines) - IPC types and structures
5. task_security.md (63 lines) - Security enforcement

Timeline: 4-6 hours (one focused session)
Compliance gain: 6% → 25%
Quality: High (manual review ensures correctness)
```

### Phase 2: Follow-up (Tier 2+3 - Next Sessions)
**Option C: Automation + Review**

```
Once Tier 1 is complete, use automation for remaining 25 modules.
Create a Python script to:
  - Batch rename files
  - Call translation API
  - Generate compliance reports

Timeline: 8-10 hours (with automation help)
Compliance gain: 25% → 100%
Quality: Good (with spot-checking)
```

---

## Prioritization Rationale

### Why Tier 1 First?

These 5 modules are **architecturally critical**:

| Module | Importance | Why |
|--------|-----------|-----|
| **task_sign** | CRITICAL | Core signing pipeline; defines FSM |
| **crypto_wallet** | CRITICAL | Cryptography integration; BIP-39/32/ECDSA |
| **task_net** | HIGH | HTTP server; transaction reception |
| **wallet_shared** | HIGH | IPC types; shared data structures |
| **task_security** | HIGH | Security enforcement; state guards |

Completing these 5 gives:
- ✅ 25% overall compliance
- ✅ 100% core module documentation
- ✅ Foundation for rest of project

### Why Tier 2 Second?

Supporting infrastructure modules:
- task_display (UI rendering)
- task_user (confirmation flow)
- usb_device (communication)
- app_ethernet (network stack)
- And 6 more

These are important for **understanding the system**, but not blocking.

### Why Tier 3 Last?

Hardware/configuration modules:
- lwipopts, ssd1306_conf
- stm32h7xx_* (interrupt handlers, HAL)
- usbd_* (USB device config)
- memzero, sha256_minimal, wallet_seed

These are **important for reference**, but not critical for core understanding.

---

## Implementation Checklist

### Before Starting

- [ ] Review `COMPLIANCE_AUDIT_SUMMARY.md` for full context
- [ ] Read `LANGUAGE_FIX_STRATEGY.md` for detailed workflow
- [ ] Ensure all 30 Russian files are backed up (git history)
- [ ] Have English/Russian technical dictionary available
- [ ] Set up Python compliance checker

### Tier 1 Execution

For each module (5 total):

- [ ] Read current Russian content
  ```bash
  cat docs_src/task_sign.md | head -50
  ```

- [ ] Manually translate to English
  - Keep Doxygen directives: `\page`, `\related`
  - Keep code examples unchanged
  - Translate sections: Abstract, Logic Flow, Dependencies
  - Translate brief tags carefully

- [ ] Save English version
  ```bash
  # Replace docs_src/task_sign.md with English content
  ```

- [ ] Create Russian version
  ```bash
  # Copy original to docs_src/task_sign_ru.md
  ```

- [ ] Create Polish version
  ```bash
  # Translate English to Polish → docs_src/task_sign_pl.md
  ```

- [ ] Verify compliance
  ```bash
  python3 scripts/check_language_compliance.py | grep task_sign
  ```

- [ ] Commit
  ```bash
  git add docs_src/task_sign*
  git commit -m "Tier 1: Translate task_sign to EN/RU/PL"
  ```

### After Tier 1

- [ ] Run full compliance check
  ```bash
  python3 scripts/check_language_compliance.py
  ```

- [ ] Update README.md index (automatic via Makefile)
  ```bash
  make docs-doxygen
  ```

- [ ] Review README.md for correct module descriptions

---

## Success Criteria

### Tier 1 Complete ✅

```
Compliance check output:
  task_sign:      ✅ OK (3 files: EN/RU/PL)
  crypto_wallet:  ✅ OK (3 files: EN/RU/PL)
  task_net:       ✅ OK (3 files: EN/RU/PL)
  wallet_shared:  ✅ OK (3 files: EN/RU/PL)
  task_security:  ✅ OK (3 files: EN/RU/PL)
  
  Overall: 7/32 modules (22%) ✅
```

### Tier 2 Complete ✅

```
  Overall: 17/32 modules (53%) ✅
```

### Tier 3 Complete ✅

```
  Overall: 32/32 modules (100%) ✅ ALL COMPLIANT
```

---

## Tooling Support

### Automated Verification

```bash
# Check compliance at any time
python3 scripts/check_language_compliance.py

# Output will show:
# - Files with language mismatches
# - Missing brief tags
# - Missing Doxygen directives
# - Encoding issues
```

### Documentation Update

```bash
# Automatically update README.md indices
make docs-doxygen

# This runs: python3 scripts/update_docs_src_index.py
```

---

## Terminology Guide (For Translation)

### Key Technical Terms

| Russian | English | Notes |
|---------|---------|-------|
| Модуль | Module | General software module |
| Задача | Task | FreeRTOS task |
| Очередь | Queue | FreeRTOS queue |
| Событие | Event | FreeRTOS event group |
| Мьютекс | Mutex | FreeRTOS mutex |
| Прерывание | Interrupt | Hardware interrupt |
| Регистр | Register | Hardware register |
| Транзакция | Transaction | Bitcoin transaction |
| Подпись | Signature | Digital signature |
| Валидация | Validation | Input validation |
| Таймаут | Timeout | Timeout interval |
| Буфер | Buffer | Memory buffer |
| Состояние | State | FSM state |
| Логирование | Logging | Debug logging |
| Криптография | Cryptography | Crypto operations |

### Domain-Specific Terms

| Russian | English | Context |
|---------|---------|---------|
| SHA-256 | SHA-256 | Hash algorithm (keep as-is) |
| ECDSA | ECDSA | Signing algorithm (keep as-is) |
| BIP-39/32 | BIP-39/32 | Bitcoin standards (keep as-is) |
| trezor-crypto | trezor-crypto | Library name (keep as-is) |
| FreeRTOS | FreeRTOS | RTOS name (keep as-is) |
| LwIP | LwIP | TCP/IP stack (keep as-is) |
| STM32 | STM32 | Microcontroller (keep as-is) |
| WebUSB | WebUSB | USB protocol (keep as-is) |

---

## Risk Assessment

### Low Risk (Safe to Proceed)

- ✅ Translation from Russian to English (all files have Russian originals)
- ✅ Following established template from hw_init/main modules
- ✅ Git history preserves all changes
- ✅ Automated verification available

### Medium Risk (Monitor)

- ⚠️ Technical terminology accuracy
  - *Mitigation:* Use provided terminology guide
  - *Check:* Domain expert review of crypto/FreeRTOS terms

- ⚠️ Brief tag quality
  - *Mitigation:* Keep briefs as single paragraphs
  - *Check:* Compliance script validates format

### No Risk (Already Mitigated)

- ✅ Doxygen directives preserved (copy-paste from original)
- ✅ Code examples unchanged (not translated)
- ✅ UTF-8 encoding (Python handles automatically)
- ✅ Git rollback possible (if major issues)

---

## FAQ

### Q: Can we batch-translate all 30 at once?
**A:** Not recommended. Start with Tier 1 (5 modules) to establish a quality baseline. Use lessons learned for Tier 2+3.

### Q: What if I make translation mistakes?
**A:** Git preserves history. You can:
1. Fix individual files and recommit
2. Revert entire commit if major issues
3. Use `git diff` to compare before/after

### Q: Should code comments be translated?
**A:** No. Only translate documentation (Markdown). Code comments in `.c`/`.h` files stay in English.

### Q: Is Polish translation critical?
**A:** Polish translation is part of the multi-language strategy, but English is the priority. If time-constrained, you can complete EN/RU first and defer PL to later.

### Q: How long does each module take?
**A:** ~45 minutes average:
- 15 min: Read Russian, understand content
- 20 min: Manually translate to English
- 10 min: Create RU/PL versions
- 5 min: Verify with compliance checker
- 5 min: Commit

Tier 1 (5 modules) = 3.75 hours

---

## Next Steps

### Immediate (Today)

1. ✅ Read this document (5 min)
2. ✅ Review `COMPLIANCE_AUDIT_SUMMARY.md` (10 min)
3. ⏳ Decide: Start immediately or schedule for next session?

### If Starting Today

1. ⏳ Open `docs_src/task_sign.md`
2. ⏳ Translate to English manually
3. ⏳ Save as versions: task_sign.md, task_sign_ru.md, task_sign_pl.md
4. ⏳ Run compliance check
5. ⏳ Commit

### If Scheduling for Later

1. ⏳ Create calendar reminder for next work session
2. ⏳ Set aside 4-6 hours for Tier 1
3. ⏳ Prepare terminology reference
4. ⏳ Begin with task_sign.md

---

## Resources

**Documentation:**
- `COMPLIANCE_AUDIT_SUMMARY.md` — Executive summary
- `LANGUAGE_COMPLIANCE_AUDIT.md` — Module-by-module breakdown
- `LANGUAGE_FIX_STRATEGY.md` — Implementation details
- `TRANSLATION_GUIDE.md` — Translation methodology
- `PHASE2_STATUS.md` — Phase 2 progress tracking

**Tools:**
- `scripts/check_language_compliance.py` — Verification
- `scripts/update_docs_src_index.py` — README index generation

**Files to Translate (Tier 1):**
- `docs_src/task_sign.md` (82 lines)
- `docs_src/crypto_wallet.md` (71 lines)
- `docs_src/task_net.md` (78 lines)
- `docs_src/wallet_shared.md` (61 lines)
- `docs_src/task_security.md` (63 lines)

---

## Conclusion

The audit is complete. The path forward is clear:

1. **Phase A (Tier 1):** 5 critical modules → 4-6 hours → 25% compliance
2. **Phase B (Tier 2+3):** 25 supporting modules → 20-25 hours → 100% compliance

Starting with Tier 1 provides:
- ✅ Quick win (22% → 25% compliance)
- ✅ Foundation for automation
- ✅ Quality baseline for remaining work
- ✅ One focused work session

**Recommendation: Begin Tier 1 translations in the next session.**

---

**Status:** 📋 Audit Complete  
**Priority:** 🔴 CRITICAL  
**Next Action:** Review recommendations, then begin Tier 1  
**Timeline:** 4-6 hours to fix critical modules
