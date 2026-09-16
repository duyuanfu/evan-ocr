## ADDED Requirements

### Requirement: Screenshot Selection OCR Trigger
The system SHALL provide an entry point in the floating toolbar to trigger OCR recognition on the currently selected screenshot region.

#### Scenario: User clicks OCR button on toolbar
- **WHEN** the user selects an area on screen and clicks the `🔤` (OCR) button on the floating toolbar
- **THEN** the system SHALL crop the selected area, show a loading indicator or status hint, and execute OCR recognition asynchronously.

### Requirement: OCR Result Dialog and Text Interaction
The system SHALL display an `OcrResultDialog` presenting the recognized text with formatting and clipboard options.

#### Scenario: Display recognized text and copy
- **WHEN** OCR recognition finishes successfully
- **THEN** the system SHALL display the recognized text in a dialog with options to copy raw text, copy with paragraph merging, or close.

#### Scenario: Visual bounding box overlay preview
- **WHEN** the result dialog is active
- **THEN** the dialog SHALL provide a preview mode showing word/line bounding boxes overlaid on the cropped image.
