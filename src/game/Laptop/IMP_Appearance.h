#ifndef IMP_APPEARANCE_H
#define IMP_APPEARANCE_H

#include "Types.h"

struct MERCPROFILESTRUCT;

// The "I.M.P. Colors and Body Type" page, shown after the name/nickname/gender
// ("I.M.P. Registry") page.
void EnterIMPAppearance(void);
void RenderIMPAppearance(void);
void ExitIMPAppearance(void);
void HandleIMPAppearance(void);

// Back to the defaults (also forgets which gender the selection was made for).
void ResetImpAppearance(void);

// Writes the selected skin/hair/shirt/pants palettes into the profile.
void ApplyImpAppearanceToProfile(MERCPROFILESTRUCT& p);

// Only meaningful for a male character.
BOOLEAN ImpAppearanceIsBigBody(void);

// "Alternative rifle holding": only ever true for a male big body.
BOOLEAN ImpAppearanceUsesAltRifleHold(void);

#endif
