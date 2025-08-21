# Third-Party Credits & Licensing Disclosures

Griddy incorporates several open source software components. This document provides the required license attributions and acknowledgments.

---

## JUCE Framework

**Website:** https://juce.com/  
**License:** ISC License  
**Copyright:** © 2020 Raw Material Software Limited

```
Permission to use, copy, modify, and/or distribute this software for any purpose with or 
without fee is hereby granted, provided that the above copyright notice and this permission 
notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO 
THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT 
SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR 
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION 
OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE 
USE OR PERFORMANCE OF THIS SOFTWARE.
```

**Used for:** Cross-platform audio plugin framework, GUI components, audio processing

---

## Mutable Instruments Grids

**IMPORTANT:** This project incorporates rhythm patterns and code from Grids by Mutable Instruments, which is licensed under the GNU General Public License v3.0. As a result, **this entire project must be licensed under GPL v3.0 or later**.

**Original Project:** [Grids by Mutable Instruments](https://github.com/pichenettes/eurorack/tree/master/grids)  
**License:** GNU General Public License v3.0  
**Copyright:** © 2012-2015 Emilie Gillet (emilie.o.gillet@gmail.com)

**Used for:** Drum pattern generation algorithms, rhythm patterns obtained through machine learning from drum loops, and pattern interpolation logic.

The original Grids module uses a 5x5 grid of drum patterns that were obtained through unsupervised learning from a large collection of drum loops. This project uses these same patterns and algorithms.

---

## Additional Acknowledgments

### Original Inspiration

Griddy is inspired by and incorporates code from **Grids** by [Mutable Instruments](https://pichenettes.github.io/mutable-instruments-documentation/modules/grids/open_source/).

### Build and Development Tools

- **CMake** - BSD 3-Clause License (build system)
- **Xcode** - Apple Developer Tools (development environment)

### Third-Party Services

- **Apple Developer Program** - Code signing and distribution platform

---

## Contact

For questions about these licenses or to report any licensing concerns, please contact:  
thegenerouscorp@gmail.com

---

## License Compliance Statement

**Griddy is licensed under the GNU General Public License v3.0** due to its incorporation of GPL-licensed code from Mutable Instruments Grids.

This means:
- **Source code must be made available** when distributing the plugin
- **Modifications must also be licensed under GPL v3.0**
- **Attribution and copyright notices must be preserved**
- **The complete license text must be included with distributions**

Where required:
- **Source code availability:** This project's source code must be publicly available
- **License preservation:** Original license texts are included in this document
- **Attribution:** Copyright notices and attributions are maintained

Last updated: August 20, 2025