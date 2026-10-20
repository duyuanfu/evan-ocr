---
name: ui-ux-pro-max
description: Comprehensive UI/UX design engineering, modern flat/Fluent design system, dark/light theme mastery, micro-interaction polish, and component aesthetics for desktop and web applications.
---

# UI/UX Pro Max Design System & Component Polish Skill

Use this skill whenever designing, reviewing, refactoring, or polishing user interfaces, dialogs, components, stylesheets, and visual layouts. It ensures software achieves tier-1 consumer-grade aesthetics, accessibility, tactility, and visual consistency across all themes and resolutions.

---

## 1. Core Visual Principles & Design Philosophy

### 1.1 Modern Flat & Fluent Design (扁平化与立体层次)
- **Flat Surface with Subtle Elevation**: Avoid overly skeuomorphic textures, but never sacrifice component affordance (可辨识度). A button must always look unmistakably like a clickable button; a combobox must unmistakably look like a selectable dropdown.
- **Surface Hierarchy**:
  - **Base Background**: Ground layer (`#18181b` in dark mode, `#f8fafc` in light mode).
  - **Card Surface**: Elevated container (`#27272a` in dark mode, `#ffffff` in light mode), with a subtle 1px border (`#3f3f46` in dark, `#e2e8f0` in light) and 8–10px corner radius.
  - **Inner Action Elements**: Nested inside cards, buttons and inputs must have a distinct background (`#38383e` in dark, `#f8fafc`/`#ffffff` in light) and 1px border to never blend into card surfaces.

### 1.2 The "Zero-White-Bleed" Dark Mode Rule (暗色模式无白斑准则)
- **Inherited Palette Danger**: In desktop frameworks (especially Qt), default system palettes will bleed pure white (`#ffffff`) into child viewports (e.g., `QScrollArea::viewport`, unstyled `QWidget` canvas, `QComboBox` popups) unless explicitly styled.
- **Rule**: Every container inside a dark-mode dialog must either be styled with the appropriate dark background or explicitly configured with `background: transparent;`.
- **Selector Precision**: Never set `parentWidget->setStyleSheet("background: transparent;")` without an ID selector (e.g., use `QWidget#contentWidget { background: transparent; }`). Setting raw `background: transparent;` cascades to all descendant popups and makes combobox dropdowns completely transparent.

---

## 2. Component Design Engineering

### 2.1 Comboboxes (下拉框规范)
- **Affordance**: A combobox must have a clear, crisp down-chevron (`⌄` / `▼`) indicator on the right.
- **Dedicated Dropdown Area**: Dedicate a 24–28px right-aligned button slot with an optional subtle vertical border divider.
- **Popup List (`QAbstractItemView` / Dropdown Menu)**:
  - Must have a 100% solid, opaque background.
  - Items must have comfortable row heights (26–30px), horizontal padding (8–12px), and rounded hover/selected states.
  - In Qt: always call `combo->setView(new QListView(combo))` to ensure platform-independent item styling and eliminate Windows native popup transparency glitches.

### 2.2 Buttons & Action Affordances (按钮与交互质感)
- **Primary Buttons**: Vibrant brand color (e.g., `#2563eb` / `#3b82f6`), pure white bold text, distinct hover (`#1d4ed8`) and pressed states.
- **Card-Embedded Secondary Buttons**: When placed inside an already-elevated card, the button must use `#38383e` (dark) or `#ffffff` with a clear border (light) so it never disappears into the card.
- **Hover & Feedback**: Every interactive button must have `setCursor(Qt::PointingHandCursor)`, subtle color transitions, and visible `:disabled` styling (`opacity: 0.5` or subdued text/border).

### 2.3 Scrollbars & Viewports (极简微悬浮滚动条)
- **Minimalist Native-like Scrollbars**:
  - Track width: 6px to 8px, 100% transparent track (no thick grey gutter).
  - Thumb: pill-shaped capsule (`border-radius: 3px`), subdued zinc color (`#52525b` dark, `#cbd5e1` light), expanding slightly on hover (`#71717a` / `#94a3b8`).
  - Arrows: completely remove top/bottom arrow buttons (`height: 0px`).

### 2.4 Segmented Controls & Tab Bars (分段式胶囊选择器)
- Replace bulky browser-tab headers with modern Windows 11 Fluent segmented pill switchers.
- Base track: pill capsule background with 3–4px padding.
- Unselected tabs: clean muted text with soft hover state.
- Active tab: elevated pill with vibrant accent background or solid card surface.

---

## 3. Typography & Spacing Hierarchy

| Token | Size | Weight | Usage |
| :--- | :--- | :--- | :--- |
| **Display Header** | 18px | 700 (Bold) | Dialog titles, hero banners (`Segoe UI Variable Display`) |
| **Card Header** | 14px–15px | 600 (Semibold) | Primary section and card titles |
| **Body Primary** | 13px | 400–500 | Form labels, button text, table contents |
| **Secondary Caption** | 11px–12px | 400 | Subtitles, input placeholders, description notes |
| **Monospace / Code** | 12px–13px | 400 | LaTeX, code snippets, logs (`Cascadia Code`, `Consolas`) |

- **Spacing Rhythm**: 4px micro-gap, 8px standard gap, 12–16px card padding, 20–24px dialog outer margins.

---

## 4. Checklist for UI/UX Reviews

- [ ] Does every dropdown look unmistakably like a combobox with a visible chevron?
- [ ] Are popup item lists 100% opaque without see-through transparency?
- [ ] In dark mode, are there any white boxes, light gutters, or harsh outlines?
- [ ] Do secondary buttons inside cards have tactile boundaries rather than floating text?
- [ ] Are scrollbars slim, elegant, and free of clunky arrow boxes?
- [ ] Are hover, pressed, focus, and disabled states implemented for all inputs?
- [ ] Is subpixel font antialiasing enabled for 4K / High DPI scaling?
