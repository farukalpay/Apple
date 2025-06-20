# macOS Structural Notarization Evasion via Ordinal-Algebraic Behaviour  
(Cumulative φ-Entropy Drift)

**Author:** Faruk Alpay  
**Date:** May 2025  
**Submission ID:** _new case_  

---

## 🧠 Summary
This PoC shows that notarization trust can be gradually “drifted” by repeatedly
feeding structurally valid yet entropy-shifted Mach-O binaries. Apple’s
Gatekeeper de-scopes them as identical and allows silent execution.

---

## ✅ Key Findings
- Entropy-driven binary morphing bypasses notarization cache
- No entitlements, no user prompts, SIP enabled
- Works on macOS 15.5 (Apple Silicon) and 14.x

---

## 📂 Files

| File | Purpose |
|------|---------|
| `notary-scan.swift` | CLI PoC – submits, polls, and retrieves notarization tickets |
| `phi_summary.csv`  | Drift metrics per build iteration |
| `risk_plot.png`    | Visual depiction of φ-entropy over time |
| `phi_report.md`    | Full technical write-up |
| `README.md`        | **← this file** |

*(If provided)*  
| `DriftScanApp.app` | Ad-hoc signed executable bundle |

---

## 🔄 Repro Steps

```bash
# build
swift build -c release

# run drift scan (creates 5 morphs, submits each)
.build/release/DriftScanApp \
    --source ~/Downloads/base.bin \
    --iterations 5
````

Expected: last two morphs execute without Gatekeeper dialog although never
explicitly notarised.

---

## 🔐 Security Impact

Shows that notarization trust is tied to structural hashes that can be nudged
without invalidating the ticket, enabling persistent unsigned code execution.

---

## 🧪 System Configuration

* MacBook M2 / macOS 15.5 (Sonoma 14.5 UI)
* SIP enabled, standard user
* Xcode 15.4 / Swift 5.9

---

## 📬 Submission Statement

Source + ad-hoc binary provided. Please assign a CVE if eligible.

— Faruk Alpay