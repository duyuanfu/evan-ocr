## ADDED Requirements

### Requirement: Floating Toolbar OCR Action
The floating toolbar SHALL provide an OCR action button to trigger character recognition on the selected area.

#### Scenario: User activates OCR from floating toolbar
- **WHEN** the user clicks the `🔤` button on the floating toolbar
- **THEN** the overlay SHALL emit a signal requesting OCR recognition for the cropped selection.
