## ADDED Requirements

### Requirement: Pin Window OCR Context Action
The pin window context menu SHALL provide an action to recognize text in the currently pinned image.

#### Scenario: User requests OCR from pin window
- **WHEN** the user right-clicks a pin window and selects "识别文字 (Ctrl+O)"
- **THEN** the system SHALL execute OCR on the pinned image and open the OCR result dialog.
