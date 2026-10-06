#include "ToneDef.h"

// 音色名
const array<string, 128> kToneName = {
    "Acoustic Piano",
    "Bright Piano",
    "Electric Grand Piano",
    "Honky-tonk Piano",
    "Electric Piano",
    "Electric Piano 2",
    "Harpsichord",
    "Clavi",
    "Celesta",
    "Glockenspiel",
    "Musical box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bell",
    "Dulcimer",
    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church organ",
    "Reed organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",
    "Acoustic Guitar (nylon)",
    "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)",
    "Electric Guitar (clean)",
    "Electric Guitar (muted)",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar harmonics",
    "Acoustic Bass",
    "Electric Bass (finger)",
    "Electric Bass (pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",
    "Violin",
    "Viola",
    "Cello",
    "Double bass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",
    "String Ensemble 1",
    "String Ensemble 2",
    "Synth Strings 1",
    "Synth Strings 2",
    "Voice Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French horn",
    "Brass Section",
    "Synth Brass 1",
    "Synth Brass 2",
    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet",
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",
    "Lead 1 (square)",
    "Lead 2 (sawtooth)",
    "Lead 3 (calliope)",
    "Lead 4 (chiff)",
    "Lead 5 (charang)",
    "Lead 6 (voice)",
    "Lead 7 (fifths)",
    "Lead 8 (bass + lead)",
    "Pad 1 (Fantasia)",
    "Pad 2 (warm)",
    "Pad 3 (polysynth)",
    "Pad 4 (choir)",
    "Pad 5 (bowed)",
    "Pad 6 (metallic)",
    "Pad 7 (halo)",
    "Pad 8 (sweep)",
    "FX 1 (rain)",
    "FX 2 (soundtrack)",
    "FX 3 (crystal)",
    "FX 4 (atmosphere)",
    "FX 5 (brightness)",
    "FX 6 (goblins)",
    "FX 7 (echoes)",
    "FX 8 (sci-fi)",
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bagpipe",
    "Fiddle",
    "Shanai",
    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal ",
    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot",
};
// パーカッション名
const array<string, 128> kDrumName = {
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Snare Roll",
    "Finger Snap",
    "High Q",
    "Slap",
    "Scratch Push",
    "Scratch Pull",
    "Sticks",
    "Square Click",
    "Metronome Click",
    "Metronome Bell",
    "Bass Drum 2",
    "Bass Drum 1",
    "Side Stick",
    "Snare Drum 1",
    "Hand Clap",
    "Snare Drum 2",
    "Low Tom 2",
    "Closed Hi-hat",
    "Low Tom 1",
    "Pedal Hi-hat",
    "Mid Tom 2",
    "Open Hi-hat",
    "Mid Tom 1",
    "High Tom 2",
    "Crash Cymbal 1",
    "High Tom 1",
    "Ride Cymbal 1",
    "Chinese Cymbal",
    "Ride Bell",
    "Tambourine",
    "Splash Cymbal",
    "Cowbell",
    "Crash Cymbal 2",
    "Vibra Slap",
    "Ride Cymbal 2",
    "High Bongo",
    "Low Bongo",
    "Mute High Conga",
    "Open High Conga",
    "Low Conga",
    "High Timbale",
    "Low Timbale",
    "High Agogo",
    "Low Agogo",
    "Cabasa",
    "Maracas",
    "Short Whistle",
    "Long Whistle",
    "Short Guiro",
    "Long Guiro",
    "Claves",
    "High Wood Block",
    "Low Wood Block",
    "Mute Cuica",
    "Open Cuica",
    "Mute Triangle",
    "Open Triangle",
    "Shaker",
    "Jingle Bell",
    "Bell Tree",
    "Castanets",
    "Mute Surdo",
    "Open Surdo",
    "Low Whistle",
    "Mute Cuica",
    "Open Cuica",
    "Mute Triangle",
    "Open Triangle",
    "Short Guiro",
    "Long Guiro",
    "Cabasa Up",
    "Cabasa Down",
    "Claves",
    "High Wood Block",
    "Low Wood Block",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};
// ====音色定義=====
// 音色定義ノイズ
const array<string, 128> kToneDefNoise = {
    "d",                                                                        //   0:
    "d",                                                                        //   1:
    "d",                                                                        //   2:
    "d",                                                                        //   3:
    "d",                                                                        //   4:
    "d",                                                                        //   5:
    "d",                                                                        //   6:
    "d",                                                                        //   7:
    "d",                                                                        //   8:
    "d",                                                                        //   9:
    "d",                                                                        //  10:
    "d",                                                                        //  11:
    "d",                                                                        //  12:
    "d",                                                                        //  13:
    "d",                                                                        //  14:
    "d",                                                                        //  15:
    "d",                                                                        //  16:
    "d",                                                                        //  17:
    "d",                                                                        //  18:
    "d",                                                                        //  19:
    "d",                                                                        //  20:
    "d",                                                                        //  21:
    "d",                                                                        //  22:
    "d",                                                                        //  23:
    "d",                                                                        //  24:
    "d",                                                                        //  25:Snare Roll
    "d",                                                                        //  26:Finger Snap
    "d",                                                                        //  27:High Q
    "d",                                                                        //  28:Slap
    "d",                                                                        //  29:Scratch Push
    "d",                                                                        //  30:Scratch Pull
    "d",                                                                        //  31:Sticks
    "d",                                                                        //  32:Square Click
    "d",                                                                        //  33:Metronome Click
    "D16c",                                                                     //  34:Metronome Bell
    "d",                                                                        //  35:Bass Drum 2
    "d",                                                                        //  36:Bass Drum 1
    "d",                                                                        //  37:Side Stick
    "d",                                                                        //  38:Snare Drum 1
    "g",                                                                        //  39:Hand Clap
    "d",                                                                        //  40:Snare Drum 2
    "d",                                                                        //  41:Low Tom 2
    "d",                                                                        //  42:Closed Hi-hat
    "d",                                                                        //  43:Low Tom 1
    "d",                                                                        //  44:Pedal Hi-hat
    "d",                                                                        //  45:Mid Tom 2
    "e",                                                                        //  46:Open Hi-hat
    "d",                                                                        //  47:Mid Tom 1
    "d",                                                                        //  48:High Tom 2
    "f+",                                                                       //  49:Crash Cymbal 1
    "d",                                                                        //  50:High Tom 1
    "d",                                                                        //  51:Ride Cymbal 1
    "g",                                                                        //  52:Chinese Cymbal
    "d",                                                                        //  53:Ride Bell
    "d",                                                                        //  54:Tambourine
    "f",                                                                        //  55:Splash Cymbal
    "d",                                                                        //  56:Cowbell
    "f",                                                                        //  57:Crash Cymbal 2
    "d",                                                                        //  58:Vibra Slap
    "c",                                                                        //  59:Ride Cymbal 2
    "d",                                                                        //  60:High Bongo
    "d",                                                                        //  61:Low Bongo
    "d",                                                                        //  62:Mute High Conga
    "d",                                                                        //  63:Open High Conga
    "d",                                                                        //  64:Low Conga
    "d",                                                                        //  65:High Timbale
    "d",                                                                        //  66:Low Timbale
    "d",                                                                        //  67:High Agogo
    "d",                                                                        //  68:Low Agogo
    "d",                                                                        //  69:Cabasa
    "d",                                                                        //  70:Maracas
    "d",                                                                        //  71:Short Whistle
    "d",                                                                        //  72:Long Whistle
    "d",                                                                        //  73:Short Guiro
    "d",                                                                        //  74:Long Guiro
    "d",                                                                        //  75:Claves
    "d",                                                                        //  76:High Wood Block
    "d",                                                                        //  77:Low Wood Block
    "d",                                                                        //  78:Mute Cuica
    "d",                                                                        //  79:Open Cuica
    "d",                                                                        //  80:Mute Triangle
    "d",                                                                        //  81:Open Triangle
    "c+",                                                                       //  82:Shaker
    "d",                                                                        //  83:Jingle Bell
    "d",                                                                        //  84:Bell Tree
    "d",                                                                        //  85:Castanets
    "d",                                                                        //  86:Mute Surdo
    "d",                                                                        //  87:Open Surdo
    "d",                                                                        //  88:Low Whistle
    "d",                                                                        //  89:Mute Cuica
    "d",                                                                        //  90:Open Cuica
    "d",                                                                        //  91:Mute Triangle
    "d",                                                                        //  92:Open Triangle
    "d",                                                                        //  93:Short Guiro
    "d",                                                                        //  94:Long Guiro
    "d",                                                                        //  95:Cabasa Up
    "d",                                                                        //  96:Cabasa Down
    "d",                                                                        //  97:Claves
    "d",                                                                        //  98:High Wood Block
    "d",                                                                        //  99:Low Wood Block
    "d",                                                                        // 100:
    "d",                                                                        // 101:
    "d",                                                                        // 102:
    "d",                                                                        // 103:
    "d",                                                                        // 104:
    "d",                                                                        // 105:
    "d",                                                                        // 106:
    "d",                                                                        // 107:
    "d",                                                                        // 108:
    "d",                                                                        // 109:
    "d",                                                                        // 110:
    "d",                                                                        // 111:
    "d",                                                                        // 112:
    "d",                                                                        // 113:
    "d",                                                                        // 114:
    "d",                                                                        // 115:
    "d",                                                                        // 116:
    "d",                                                                        // 117:
    "d",                                                                        // 118:
    "d",                                                                        // 119:
    "d",                                                                        // 120:
    "d",                                                                        // 121:
    "d",                                                                        // 122:
    "d",                                                                        // 123:
    "d",                                                                        // 124:
    "d",                                                                        // 125:
    "d",                                                                        // 126:
    "d",                                                                        // 127:
};
// 音色定義矩形波
const array<string, 128> kToneDefSquare = {
    "2",                                                                        //   1:Acoustic Piano
    "2",                                                                        //   2:Bright Piano
    "2",                                                                        //   3:Electric Grand Piano
    "2",                                                                        //   4:Honky-tonk Piano
    "2",                                                                        //   5:Electric Piano
    "2",                                                                        //   6:Electric Piano 2
    "2",                                                                        //   7:Harpsichord
    "2",                                                                        //   8:Clavi
    "2",                                                                        //   9:Celesta
    "2",                                                                        //  10:Glockenspiel
    "2",                                                                        //  11:Musical box
    "2",                                                                        //  12:Vibraphone
    "2",                                                                        //  13:Marimba
    "2",                                                                        //  14:Xylophone
    "2",                                                                        //  15:Tubular Bell
    "",                                                                         //  16:Dulcimer
    "",                                                                         //  17:Drawbar Organ
    "",                                                                         //  18:Percussive Organ
    "",                                                                         //  19:Rock Organ
    "",                                                                         //  20:Church organ
    "",                                                                         //  21:Reed organ
    "1",                                                                        //  22:Accordion
    "1",                                                                        //  23:Harmonica
    "1",                                                                        //  24:Tango Accordion
    "2",                                                                        //  25:Acoustic Guitar (nylon)
    "1",                                                                        //  26:Acoustic Guitar (steel)
    "1",                                                                        //  27:Electric Guitar (jazz)
    "1",                                                                        //  28:Electric Guitar (clean)
    "2",                                                                        //  29:Electric Guitar (muted)
    "1",                                                                        //  30:Overdriven Guitar
    "1|0",                                                                      //  31:Distortion Guitar
    "2",                                                                        //  32:Guitar harmonics
    "",                                                                         //  33:Acoustic Bass
    "",                                                                         //  34:Electric Bass (finger)
    "",                                                                         //  35:Electric Bass (pick)
    "",                                                                         //  36:Fretless Bass
    "1",                                                                        //  37:Slap Bass 1
    "1",                                                                        //  38:Slap Bass 2
    "",                                                                         //  39:Synth Bass 1
    "",                                                                         //  40:Synth Bass 2
    "",                                                                         //  41:Violin
    "",                                                                         //  42:Viola
    "",                                                                         //  43:Cello
    "",                                                                         //  44:Double bass
    "",                                                                         //  45:Tremolo Strings
    "",                                                                         //  46:Pizzicato Strings
    "",                                                                         //  47:Orchestral Harp
    "",                                                                         //  48:Timpani
    "",                                                                         //  49:String Ensemble 1
    "",                                                                         //  50:String Ensemble 2
    "",                                                                         //  51:Synth Strings 1
    "",                                                                         //  52:Synth Strings 2
    "2",                                                                        //  53:Voice Aahs
    "2",                                                                        //  54:Voice Oohs
    "2",                                                                        //  55:Synth Voice
    "1",                                                                        //  56:Orchestra Hit
    "0",                                                                        //  57:Trumpet
    "0",                                                                        //  58:Trombone
    "0",                                                                        //  59:Tuba
    "0",                                                                        //  60:Muted Trumpet
    "0",                                                                        //  61:French horn
    "",                                                                         //  62:Brass Section
    "",                                                                         //  63:Synth Brass 1
    "",                                                                         //  64:Synth Brass 2
    "",                                                                         //  65:Soprano Sax
    "",                                                                         //  66:Alto Sax
    "",                                                                         //  67:Tenor Sax
    "",                                                                         //  68:Baritone Sax
    "",                                                                         //  69:Oboe
    "",                                                                         //  70:English Horn
    "",                                                                         //  71:Bassoon
    "2",                                                                        //  72:Clarinet
    "2",                                                                        //  73:Piccolo
    "2",                                                                        //  74:Flute
    "2",                                                                        //  75:Recorder
    "2",                                                                        //  76:Pan Flute
    "2",                                                                        //  77:Blown Bottle
    "1",                                                                        //  78:Shakuhachi
    "2",                                                                        //  79:Whistle
    "2",                                                                        //  80:Ocarina
    "2",                                                                        //  81:Lead 1 (square)
    "1",                                                                        //  82:Lead 2 (sawtooth)
    "",                                                                         //  83:Lead 3 (calliope)
    "",                                                                         //  84:Lead 4 (chiff)
    "",                                                                         //  85:Lead 5 (charang)
    "2",                                                                        //  86:Lead 6 (voice)
    "",                                                                         //  87:Lead 7 (fifths)
    "",                                                                         //  88:Lead 8 (bass + lead)
    "",                                                                         //  89:Pad 1 (Fantasia)
    "",                                                                         //  90:Pad 2 (warm)
    "2",                                                                        //  91:Pad 3 (polysynth)
    "",                                                                         //  92:Pad 4 (choir)
    "",                                                                         //  93:Pad 5 (bowed)
    "",                                                                         //  94:Pad 6 (metallic)
    "",                                                                         //  95:Pad 7 (halo)
    "",                                                                         //  96:Pad 8 (sweep)
    "",                                                                         //  97:FX 1 (rain)
    "",                                                                         //  98:FX 2 (soundtrack)
    "2",                                                                        //  99:FX 3 (crystal)
    "2",                                                                        // 100:FX 4 (atmosphere)
    "",                                                                         // 101:FX 5 (brightness)
    "",                                                                         // 102:FX 6 (goblins)
    "",                                                                         // 103:FX 7 (echoes)
    "",                                                                         // 104:FX 8 (sci-fi)
    "",                                                                         // 105:Sitar
    "",                                                                         // 106:Banjo
    "0 1 0",                                                                    // 107:Shamisen
    "1",                                                                        // 108:Koto
    "2",                                                                        // 109:Kalimba
    "",                                                                         // 110:Bagpipe
    "",                                                                         // 111:Fiddle
    "",                                                                         // 112:Shanai
    "",                                                                         // 113:Tinkle Bell
    "",                                                                         // 114:Agogo
    "",                                                                         // 115:Steel Drums
    "",                                                                         // 116:Woodblock
    "",                                                                         // 117:Taiko Drum
    "",                                                                         // 118:Melodic Tom
    "",                                                                         // 119:Synth Drum
    "",                                                                         // 120:Reverse Cymbal
    "",                                                                         // 121:Guitar Fret Noise
    "",                                                                         // 122:Breath Noise
    "",                                                                         // 123:Seashore
    "",                                                                         // 124:Bird Tweet
    "",                                                                         // 125:Telephone Ring
    "",                                                                         // 126:Helicopter
    "",                                                                         // 127:Applause
    "",                                                                         // 128:Gunshot
};
// 音色定義VRC6
const array<string, 128> kToneDefVrc6 = {
    "",                                                                         //   1:Acoustic Piano
    "",                                                                         //   2:Bright Piano
    "",                                                                         //   3:Electric Grand Piano
    "",                                                                         //   4:Honky-tonk Piano
    "",                                                                         //   5:Electric Piano
    "",                                                                         //   6:Electric Piano 2
    "",                                                                         //   7:Harpsichord
    "",                                                                         //   8:Clavi
    "",                                                                         //   9:Celesta
    "",                                                                         //  10:Glockenspiel
    "",                                                                         //  11:Musical box
    "",                                                                         //  12:Vibraphone
    "",                                                                         //  13:Marimba
    "",                                                                         //  14:Xylophone
    "",                                                                         //  15:Tubular Bell
    "",                                                                         //  16:Dulcimer
    "",                                                                         //  17:Drawbar Organ
    "",                                                                         //  18:Percussive Organ
    "",                                                                         //  19:Rock Organ
    "",                                                                         //  20:Church organ
    "",                                                                         //  21:Reed organ
    "",                                                                         //  22:Accordion
    "",                                                                         //  23:Harmonica
    "",                                                                         //  24:Tango Accordion
    "",                                                                         //  25:Acoustic Guitar (nylon)
    "",                                                                         //  26:Acoustic Guitar (steel)
    "",                                                                         //  27:Electric Guitar (jazz)
    "",                                                                         //  28:Electric Guitar (clean)
    "",                                                                         //  29:Electric Guitar (muted)
    "",                                                                         //  30:Overdriven Guitar
    "",                                                                         //  31:Distortion Guitar
    "",                                                                         //  32:Guitar harmonics
    "",                                                                         //  33:Acoustic Bass
    "",                                                                         //  34:Electric Bass (finger)
    "",                                                                         //  35:Electric Bass (pick)
    "",                                                                         //  36:Fretless Bass
    "",                                                                         //  37:Slap Bass 1
    "",                                                                         //  38:Slap Bass 2
    "",                                                                         //  39:Synth Bass 1
    "",                                                                         //  40:Synth Bass 2
    "",                                                                         //  41:Violin
    "",                                                                         //  42:Viola
    "",                                                                         //  43:Cello
    "",                                                                         //  44:Double bass
    "",                                                                         //  45:Tremolo Strings
    "",                                                                         //  46:Pizzicato Strings
    "",                                                                         //  47:Orchestral Harp
    "",                                                                         //  48:Timpani
    "",                                                                         //  49:String Ensemble 1
    "",                                                                         //  50:String Ensemble 2
    "",                                                                         //  51:Synth Strings 1
    "",                                                                         //  52:Synth Strings 2
    "",                                                                         //  53:Voice Aahs
    "",                                                                         //  54:Voice Oohs
    "",                                                                         //  55:Synth Voice
    "",                                                                         //  56:Orchestra Hit
    "",                                                                         //  57:Trumpet
    "",                                                                         //  58:Trombone
    "",                                                                         //  59:Tuba
    "",                                                                         //  60:Muted Trumpet
    "",                                                                         //  61:French horn
    "",                                                                         //  62:Brass Section
    "",                                                                         //  63:Synth Brass 1
    "",                                                                         //  64:Synth Brass 2
    "",                                                                         //  65:Soprano Sax
    "",                                                                         //  66:Alto Sax
    "",                                                                         //  67:Tenor Sax
    "",                                                                         //  68:Baritone Sax
    "",                                                                         //  69:Oboe
    "",                                                                         //  70:English Horn
    "",                                                                         //  71:Bassoon
    "",                                                                         //  72:Clarinet
    "",                                                                         //  73:Piccolo
    "",                                                                         //  74:Flute
    "",                                                                         //  75:Recorder
    "",                                                                         //  76:Pan Flute
    "",                                                                         //  77:Blown Bottle
    "",                                                                         //  78:Shakuhachi
    "",                                                                         //  79:Whistle
    "",                                                                         //  80:Ocarina
    "",                                                                         //  81:Lead 1 (square)
    "",                                                                         //  82:Lead 2 (sawtooth)
    "",                                                                         //  83:Lead 3 (calliope)
    "",                                                                         //  84:Lead 4 (chiff)
    "",                                                                         //  85:Lead 5 (charang)
    "",                                                                         //  86:Lead 6 (voice)
    "",                                                                         //  87:Lead 7 (fifths)
    "",                                                                         //  88:Lead 8 (bass + lead)
    "",                                                                         //  89:Pad 1 (Fantasia)
    "",                                                                         //  90:Pad 2 (warm)
    "",                                                                         //  91:Pad 3 (polysynth)
    "",                                                                         //  92:Pad 4 (choir)
    "",                                                                         //  93:Pad 5 (bowed)
    "",                                                                         //  94:Pad 6 (metallic)
    "",                                                                         //  95:Pad 7 (halo)
    "",                                                                         //  96:Pad 8 (sweep)
    "",                                                                         //  97:FX 1 (rain)
    "",                                                                         //  98:FX 2 (soundtrack)
    "",                                                                         //  99:FX 3 (crystal)
    "",                                                                         // 100:FX 4 (atmosphere)
    "",                                                                         // 101:FX 5 (brightness)
    "",                                                                         // 102:FX 6 (goblins)
    "",                                                                         // 103:FX 7 (echoes)
    "",                                                                         // 104:FX 8 (sci-fi)
    "",                                                                         // 105:Sitar
    "",                                                                         // 106:Banjo
    "",                                                                         // 107:Shamisen
    "",                                                                         // 108:Koto
    "",                                                                         // 109:Kalimba
    "",                                                                         // 110:Bagpipe
    "",                                                                         // 111:Fiddle
    "",                                                                         // 112:Shanai
    "",                                                                         // 113:Tinkle Bell
    "",                                                                         // 114:Agogo
    "",                                                                         // 115:Steel Drums
    "",                                                                         // 116:Woodblock
    "",                                                                         // 117:Taiko Drum
    "",                                                                         // 118:Melodic Tom
    "",                                                                         // 119:Synth Drum
    "",                                                                         // 120:Reverse Cymbal
    "",                                                                         // 121:Guitar Fret Noise
    "",                                                                         // 122:Breath Noise
    "",                                                                         // 123:Seashore
    "",                                                                         // 124:Bird Tweet
    "",                                                                         // 125:Telephone Ring
    "",                                                                         // 126:Helicopter
    "",                                                                         // 127:Applause
    "",                                                                         // 128:Gunshot
};
// VRC7ユーザー音色
const array<string, 128> kToneDefVrc7User = {
    "$11,$11,$8F,$4A,$C1,$F2,$33,$21",                                          //  1:Acoustic Piano
    "$11,$11,$92,$0C,$F1,$F2,$33,$21",                                          //  2:Bright Piano
    "$02,$01,$1C,$20,$F4,$F0,$21,$21",                                          //  3:Electric Grand Piano
    "$11,$11,$16,$4C,$F0,$F2,$01,$21",                                          //  4:Honky-tonk Piano
    "$02,$01,$1C,$20,$F4,$F0,$21,$21",                                          //  5:Electric Piano
    "$15,$11,$9C,$86,$F2,$F2,$31,$21",                                          //  6:Electric Piano 2
    "$01,$04,$00,$1A,$D0,$F4,$11,$21",                                          //  7:Harpsichord
    //  "$11,$D1,$8A,$0C,$d1,$F1,$32,$11"
    "$01,$04,$00,$1A,$D0,$F4,$11,$21",                                          //  8:Clavi
    //  "$11,$D2,$8A,$0C,$F2,$F2,$32,$42"
    "$09,$01,$CB,$03,$B6,$F2,$41,$41",                                          //  9:Celesta
    //  "$17,$11,$18,$00,$F2,$F3,$31,$41"
    "$09,$04,$CB,$03,$B6,$F2,$41,$41",                                          // 10:Glockenspiel
    "$07,$01,$20,$00,$75,$F4,$53,$50",                                          // 11:Musical box
    "$0B,$02,$1C,$4F,$F4,$F5,$41,$31",                                          // 12:Vibraphone
    // "$0B,$02,$1C,$4F,$F4,$F5,$41,$31"
    "$15,$11,$9C,$06,$F2,$F2,$31,$21",                                          // 13:Marimba
    "",                                                                         // 14:Xylophone
    "",                                                                         // 15:Tubular Bell
    "$08,$02,$14,$01,$C2,$CA,$24,$33",                                          // 16:Dulcimer
    "",                                                                         // 17:Drawbar Organ
    "",                                                                         // 18:Percussive Organ
    "",                                                                         // 19:Rock Organ
    "",                                                                         // 20:Church organ
    "",                                                                         // 21:Reed organ
    "",                                                                         // 22:Accordion
    "",                                                                         // 23:Harmonica
    "",                                                                         // 24:Tango Accordion
    "$11,$11,$91,$04,$F1,$F2,$33,$21",                                          // 25:Acoustic Guitar (nylon)
    "$11,$11,$91,$04,$F1,$F2,$33,$21",                                          // 26:Acoustic Guitar (steel)
    "",                                                                         // 27:Electric Guitar (jazz)
    "",                                                                         // 28:Electric Guitar (clean)
    "",                                                                         // 29:Electric Guitar (muted)
    "$01,$04,$03,$04,$71,$92,$21,$11",                                          // 30:Overdriven Guitar
    "$31,$37,$00,$13,$52,$F1,$18,$1C",                                          // 31:Distortion Guitar
    "",                                                                         //  32:Guitar harmonics
    "",                                                                         //  33:Acoustic Bass
    "",                                                                         //  34:Electric Bass (finger)
    "",                                                                         //  35:Electric Bass (pick)
    "",                                                                         //  36:Fretless Bass
    "",                                                                         //  37:Slap Bass 1
    "",                                                                         //  38:Slap Bass 2
    "$11,$03,$50,$06,$A4,$D4,$44,$41",                                          //  39:Synth Bass 1
    "$11,$03,$50,$05,$A5,$D4,$44,$41",                                          //  40:Synth Bass 2
    "",                                                                         //  41:Violin
    "",                                                                         //  42:Viola
    "",                                                                         //  43:Cello
    "",                                                                         //  44:Double bass
    "",                                                                         //  45:Tremolo Strings
    "",                                                                         //  46:Pizzicato Strings
    "",                                                                         //  47:Orchestral Harp
    "$01,$02,$C9,$0E,$E9,$E5,$40,$61",                                          //  48:Timpani
    "",                                                                         //  49:String Ensemble 1
    "",                                                                         //  50:String Ensemble 2
    "",                                                                         //  51:Synth Strings 1
    "",                                                                         //  52:Synth Strings 2
    "",                                                                         //  53:Voice Aahs
    "",                                                                         //  54:Voice Oohs
    "",                                                                         //  55:Synth Voice
    "",                                                                         //  56:Orchestra Hit
    "",                                                                         //  57:Trumpet
    "",                                                                         //  58:Trombone
    "",                                                                         //  59:Tuba
    "",                                                                         //  60:Muted Trumpet
    "",                                                                         //  61:French horn
    "",                                                                         //  62:Brass Section
    "",                                                                         //  63:Synth Brass 1
    "",                                                                         //  64:Synth Brass 2
    "",                                                                         //  65:Soprano Sax
    "",                                                                         //  66:Alto Sax
    "",                                                                         //  67:Tenor Sax
    "",                                                                         //  68:Baritone Sax
    "$01,$41,$0E,$04,$70,$C7,$12,$11",                                          //  69:Oboe
    "",                                                                         //  70:English Horn
    "",                                                                         //  71:Bassoon
    "$11,$41,$0E,$04,$70,$C7,$13,$10",                                          //  72:Clarinet
    "$02,$22,$00,$00,$FC,$F0,$7F,$0F",                                          //  73:Piccolo
    "$02,$21,$00,$00,$FC,$A0,$1F,$0F",                                          //  74:Flute
    "",                                                                         //  75:Recorder
    "",                                                                         //  76:Pan Flute
    "",                                                                         //  77:Blown Bottle
    "",                                                                         //  78:Shakuhachi
    "",                                                                         //  79:Whistle
    "$02,$22,$00,$00,$FC,$F0,$7F,$0F",                                          //  80:Ocarina
    "",                                                                         //  81:Lead 1 (square)
    "$01,$02,$07,$04,$71,$F1,$11,$11",                                          //  82:Lead 2 (sawtooth)
    "",                                                                         //  83:Lead 3 (calliope)
    "",                                                                         //  84:Lead 4 (chiff)
    "",                                                                         //  85:Lead 5 (charang)
    "$02,$22,$00,$00,$FC,$F0,$7F,$0F",                                          //  86:Lead 6 (voice)
    "",                                                                         //  87:Lead 7 (fifths)
    "",                                                                         //  88:Lead 8 (bass + lead)
    "",                                                                         //  89:Pad 1 (Fantasia)
    "",                                                                         //  90:Pad 2 (warm)
    "$22,$01,$00,$0A,$C6,$A6,$21,$11",                                          //  91:Pad 3 (polysynth)
    "",                                                                         //  92:Pad 4 (choir)
    "",                                                                         //  93:Pad 5 (bowed)
    "",                                                                         //  94:Pad 6 (metallic)
    "",                                                                         //  95:Pad 7 (halo)
    "",                                                                         //  96:Pad 8 (sweep)
    "",                                                                         //  97:FX 1 (rain)
    "",                                                                         //  98:FX 2 (soundtrack)
    "",                                                                         //  99:FX 3 (crystal)
    "",                                                                         // 100:FX 4 (atmosphere)
    "$15,$11,$9C,$06,$F2,$F2,$31,$21",                                          // 101:FX 5 (brightness)
    "",                                                                         // 102:FX 6 (goblins)
    "",                                                                         // 103:FX 7 (echoes)
    "",                                                                         // 104:FX 8 (sci-fi)
    "",                                                                         // 105:Sitar
    "$11,$1F,$01,$03,$B1,$B3,$10,$21",                                          // 106:Banjo
    "",                                                                         // 107:Shamisen
    "$03,$01,$00,$47,$CD,$D2,$75,$35",                                          // 108:Koto
    "$25,$21,$5B,$0E,$F6,$C9,$66,$0A",                                          // 109:Kalimba
    "",                                                                         // 110:Bagpipe
    "",                                                                         // 111:Fiddle
    "",                                                                         // 112:Shanai
    "",                                                                         // 113:Tinkle Bell
    "",                                                                         // 114:Agogo
    "$16,$C4,$1A,$03,$63,$B6,$35,$44",                                          // 115:Steel Drums
    "",                                                                         // 116:Woodblock
    "",                                                                         // 117:Taiko Drum
    "$01,$02,$C9,$0E,$E9,$E6,$40,$61",                                          // 118:Melodic Tom
    "",                                                                         // 119:Synth Drum
    "",                                                                         // 120:Reverse Cymbal
    "",                                                                         // 121:Guitar Fret Noise
    "$D0,$D0,$00,$58,$10,$10,$11,$11",                                          // 122:Breath Noise
    "$D0,$D0,$00,$58,$10,$10,$11,$11",                                          // 123:Seashore
    "",                                                                         // 124:Bird Tweet
    "",                                                                         // 125:Telephone Ring
    "$D0,$D0,$00,$58,$10,$10,$11,$11",                                          // 126:Helicopter
    "",                                                                         // 127:Applause
    "",                                                                         // 128:Gunshot
};
// VRC7プリセット音色
// 0ユーザー定義音色
// 1シンセ(ややハード)
// 2ギター(エレキ系)
// 3ピアノ(アコースティック系)ギター
// 4ストリングス　バイオリン
// 5クラリネット　オーボエ
// 6ベル系
// 7トランペット
// 8オルガン　アコーディオン　ハーモニカ
// 9ホルン
// 10オルゴール
// 11ビブラフォン
// 12鋸波
// 13アコースティックベース
// 14シンセベース１
// 15シンセベース２
const array<string, 128> kToneDefVrc7Preset = {
    "3",                                                                        //   1:Acoustic Piano
    "3",                                                                        //   2:Bright Piano
    "3",                                                                        //   3:Electric Grand Piano
    "3",                                                                        //   4:Honky-tonk Piano
    "3",                                                                        //   5:Electric Piano
    "11",                                                                       //   6:Electric Piano 2
    "12",                                                                       //   7:Harpsichord
    "12",                                                                       //   8:Clavi
    "11",                                                                       //   9:Celesta
    "6",                                                                        //  10:Glockenspiel
    "6",                                                                        //  11:Musical box
    "10",                                                                       //  12:Vibraphone
    "11",                                                                       //  13:Marimba
    "6",                                                                        //  14:Xylophone
    "6",                                                                        //  15:Tubular Bell
    "2",                                                                        //  16:Dulcimer
    "8",                                                                        //  17:Drawbar Organ
    "13",                                                                       //  18:Percussive Organ
    "13",                                                                       //  19:Rock Organ
    "8",                                                                        //  20:Church organ
    "8",                                                                        //  21:Reed organ
    "8",                                                                        //  22:Accordion
    "8",                                                                        //  23:Harmonica
    "8",                                                                        //  24:Tango Accordion
    "3",                                                                        //  25:Acoustic Guitar (nylon)
    "3",                                                                        //  26:Acoustic Guitar (steel)
    "3",                                                                        //  27:Electric Guitar (jazz)
    "2",                                                                        //  28:Electric Guitar (clean)
    "15",                                                                       //  29:Electric Guitar (muted)
    "7",                                                                        //  30:Overdriven Guitar
    "1",                                                                        //  31:Distortion Guitar
    "1",                                                                        //  32:Guitar harmonics
    "3",                                                                        //  33:Acoustic Bass
    "13",                                                                       //  34:Electric Bass (finger)
    "13",                                                                       //  35:Electric Bass (pick)
    "5",                                                                        //  36:Fretless Bass
    "14",                                                                       //  37:Slap Bass 1
    "14",                                                                       //  38:Slap Bass 2
    "14",                                                                       //  39:Synth Bass 1
    "3",                                                                        //  40:Synth Bass 2
    "4",                                                                        //  41:Violin
    "4",                                                                        //  42:Viola
    "8",                                                                        //  43:Cello
    "8",                                                                        //  44:Double bass
    "4",                                                                        //  45:Tremolo Strings
    "13",                                                                       //* 46:Pizzicato Strings
    "3",                                                                        //  47:Orchestral Harp
    "2",                                                                        //* 48:Timpani
    "4",                                                                        //  49:String Ensemble 1
    "4",                                                                        //* 50:String Ensemble 2
    "4",                                                                        //  51:Synth Strings 1
    "4",                                                                        //  52:Synth Strings 2
    "5",                                                                        //  53:Voice Aahs
    "5",                                                                        //  54:Voice Oohs
    "8",                                                                        //  55:Synth Voice
    "1",                                                                        //* 56:Orchestra Hit
    "7",                                                                        //  57:Trumpet
    "7",                                                                        //  58:Trombone
    "7",                                                                        //  59:Tuba
    "8",                                                                        //  60:Muted Trumpet
    "9",                                                                        //  61:French horn
    "7",                                                                        //  62:Brass Section
    "7",                                                                        //  63:Synth Brass 1
    "8",                                                                        //  64:Synth Brass 2
    "7",                                                                        //  65:Soprano Sax
    "7",                                                                        //  66:Alto Sax
    "7",                                                                        //  67:Tenor Sax
    "7",                                                                        //  68:Baritone Sax
    "5",                                                                        //  69:Oboe
    "8",                                                                        //  70:English Horn
    "7",                                                                        //  71:Bassoon
    "5",                                                                        //  72:Clarinet
    "5",                                                                        //  73:Piccolo
    "5",                                                                        //  74:Flute
    "5",                                                                        //  75:Recorder
    "5",                                                                        //  76:Pan Flute
    "5",                                                                        //  77:Blown Bottle
    "5",                                                                        //  78:Shakuhachi
    "5",                                                                        //  79:Whistle
    "5",                                                                        //  80:Ocarina
    "12",                                                                       //  81:Lead 1 (square)
    "12",                                                                       //  82:Lead 2 (sawtooth)
    "5",                                                                        //  83:Lead 3 (calliope)
    "7",                                                                        //* 84:Lead 4 (chiff)
    "1",                                                                        //  85:Lead 5 (charang)
    "5",                                                                        //  86:Lead 6 (voice)
    "12",                                                                       //  87:Lead 7 (fifths)
    "7",                                                                        //  88:Lead 8 (bass + lead)
    "11",                                                                       //  89:Pad 1 (Fantasia)
    "5",                                                                        //  90:Pad 2 (warm)
    "1",                                                                        //  91:Pad 3 (polysynth)
    "1",                                                                        //  92:Pad 4 (choir)
    "1",                                                                        //  93:Pad 5 (bowed)
    "1",                                                                        //  94:Pad 6 (metallic)
    "5",                                                                        //  95:Pad 7 (halo)
    "5",                                                                        //  96:Pad 8 (sweep)
    "6",                                                                        //  97:FX 1 (rain)
    "9",                                                                        //* 98:FX 2 (soundtrack)
    "10",                                                                       //  99:FX 3 (crystal)
    "3",                                                                        // 100:FX 4 (atmosphere)
    "11",                                                                       // 101:FX 5 (brightness)
    "9",                                                                        // 102:FX 6 (goblins)
    "9",                                                                        // 103:FX 7 (echoes)
    "9",                                                                        // 104:FX 8 (sci-fi)
    "7",                                                                        // 105:Sitar
    "7",                                                                        //*106:Banjo
    "1",                                                                        // 107:Shamisen
    "7",                                                                        //*108:Koto
    "10",                                                                       //*109:Kalimba
    "7",                                                                        // 110:Bagpipe
    "7",                                                                        // 111:Fiddle
    "8",                                                                        // 112:Shanai
    "6",                                                                        // 113:Tinkle Bell
    "6",                                                                        //*114:Agogo
    "6",                                                                        //*115:Steel Drums
    "10",                                                                       //*116:Woodblock
    "3",                                                                        //*117:Taiko Drum
    "6",                                                                        //*118:Melodic Tom
    "5",                                                                        //*119:Synth Drum
    "9",                                                                        // 120:Reverse Cymbal
    "12",                                                                       // 121:Guitar Fret Noise
    "11",                                                                       // 122:Breath Noise
    "11",                                                                       // 123:Seashore
    "4",                                                                        // 124:Bird Tweet
    "6",                                                                        // 125:Telephone Ring
    "4",                                                                        // 126:Helicopter
    "6",                                                                        // 127:Applause
    "6",                                                                        // 128:Gunshot
};
// 音色定義N106
const array<string, 128> kToneDefN106 = {
    "0,3,6,8,10,12,13,14,15,14,13,12,10,8,6,3",                                 //   1:Acoustic Piano
    "0,3,6,8,10,12,13,14,15,14,13,12,10,8,6,3",                                 //   2:Bright Piano
    "0,3,6,8,10,12,13,14,15,14,13,12,10,8,6,3",                                 //   3:Electric Grand Piano
    "8,10,12,14,15,14,12,10,8,5,3,1,0,1,3,5",                                   //   4:Honky-tonk Piano
    "",                                                                         //   5:Electric Piano
    "",                                                                         //   6:Electric Piano 2
    "",                                                                         //   7:Harpsichord
    "0,10,8,11,8,10,4,8,7,8,4,10,8,11,8,10",                                    //   8:Clavi
    "0,10,8,11,8,10,4,8,7,8,4,10,8,11,8,10",                                    //   9:Celesta
    "0,10,8,11,8,10,4,8,7,8,4,10,8,11,8,10",                                    //  10:Glockenspiel
    "0,10,8,11,8,10,4,8,7,8,4,10,8,11,8,10",                                    //  11:Musical box
    "8,10,12,14,15,14,12,10,8,5,3,1,0,1,3,5",                                   //  12:Vibraphone
    "8,10,12,14,15,14,12,10,8,5,3,1,0,1,3,5",                                   //  13:Marimba
    "0,10,8,11,8,10,4,8,7,8,4,10,8,11,8,10",                                    //  14:Xylophone
    "0,10,8,11,8,10,4,8,7,8,4,10,8,11,8,10",                                    //  15:Tubular Bell
    "0,10,8,11,8,10,4,8,7,8,4,10,8,11,8,10",                                    //  16:Dulcimer
    "11,15,9,13,5,10,9,8,7,6,5,10,2,6,0,4",                                     //  17:Drawbar Organ
    "11,15,9,13,5,10,9,8,7,6,5,10,2,6,0,4",                                     //  18:Percussive Organ
    "11,15,9,13,5,10,9,8,7,6,5,10,2,6,0,4",                                     //  19:Rock Organ
    "11,15,9,13,5,10,9,8,7,6,5,10,2,6,0,4",                                     //  20:Church organ
    "11,15,9,13,5,10,9,8,7,6,5,10,2,6,0,4",                                     //  21:Reed organ
    "11,15,9,13,5,10,9,8,7,6,5,10,2,6,0,4",                                     //  22:Accordion
    "11,15,9,13,5,10,9,8,7,6,5,10,2,6,0,4",                                     //  23:Harmonica
    "11,15,9,13,5,10,9,8,7,6,5,10,2,6,0,4",                                     //  24:Tango Accordion
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  25:Acoustic Guitar (nylon)
    "0,3,6,8,10,12,13,14,15,14,13,12,10,8,6,3",                                 //  26:Acoustic Guitar (steel)
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  27:Electric Guitar (jazz)
    "0,3,6,8,10,12,13,14,15,14,13,12,10,8,6,3",                                 //  28:Electric Guitar (clean)
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  29:Electric Guitar (muted)
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  30:Overdriven Guitar
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  31:Distortion Guitar
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  32:Guitar harmonics
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  33:Acoustic Bass
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  34:Electric Bass (finger)
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  35:Electric Bass (pick)
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  36:Fretless Bass
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  37:Slap Bass 1
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  38:Slap Bass 2
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  39:Synth Bass 1
    "4,10,2,7,15,0,1,6,15,11,5,3,13,6,9,12",                                    //  40:Synth Bass 2
    "",                                                                         //  41:Violin
    "",                                                                         //  42:Viola
    "",                                                                         //  43:Cello
    "",                                                                         //  44:Double bass
    "9,11,12,13,14,15,13,7,0,0,1,2,3,4,5,7",                                    //  45:Tremolo Strings
    "8,10,12,14,15,14,12,10,8,5,3,1,0,1,3,5",                                   //  46:Pizzicato Strings
    "8,10,12,14,15,14,12,10,8,5,3,1,0,1,3,5",                                   //  47:Orchestral Harp
    "",                                                                         //  48:Timpani
    "9,11,12,13,14,15,13,7,0,0,1,2,3,4,5,7",                                    //  49:String Ensemble 1
    "9,11,12,13,14,15,13,7,0,0,1,2,3,4,5,7",                                    //  50:String Ensemble 2
    "9,11,12,13,14,15,8,0,1,2,3,4,5,6,7,8",                                     //  51:Synth Strings 1
    "9,11,12,13,14,15,8,0,1,2,3,4,5,6,7,8",                                     //  52:Synth Strings 2
    "",                                                                         //  53:Voice Aahs
    "",                                                                         //  54:Voice Oohs
    "",                                                                         //  55:Synth Voice
    "",                                                                         //  56:Orchestra Hit
    "",                                                                         //  57:Trumpet
    "",                                                                         //  58:Trombone
    "",                                                                         //  59:Tuba
    "",                                                                         //  60:Muted Trumpet
    "",                                                                         //  61:French horn
    "",                                                                         //  62:Brass Section
    "8,10,12,14,15,14,12,10,8,5,3,1,0,1,3,5",                                   //  63:Synth Brass 1
    "8,10,12,14,15,14,12,10,8,5,3,1,0,1,3,5",                                   //  64:Synth Brass 2
    "",                                                                         //  65:Soprano Sax
    "",                                                                         //  66:Alto Sax
    "",                                                                         //  67:Tenor Sax
    "",                                                                         //  68:Baritone Sax
    "0,0,2,5,7,5,2,0,15,15,12,10,7,10,12,15",                                   //  69:Oboe
    "",                                                                         //  70:English Horn
    "15,12,10,7,10,12,15,15,0,2,5,7,5,2,0,0",                                   //  71:Bassoon
    "",                                                                         //  72:Clarinet
    "",                                                                         //  73:Piccolo
    "7,9,13,15,10,3,1,0,0,1,3,10,15,13,9,7",                                    //  74:Flute
    "",                                                                         //  75:Recorder
    "",                                                                         //  76:Pan Flute
    "",                                                                         //  77:Blown Bottle
    "",                                                                         //  78:Shakuhachi
    "",                                                                         //  79:Whistle
    "",                                                                         //  80:Ocarina
    "15,15,15,15,15,15,15,15,0,0,0,0,0,0,0,0",                                  //  81:Lead 1 (square)
    "15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0",                                    //  82:Lead 2 (sawtooth)
    "",                                                                         //  83:Lead 3 (calliope)
    "",                                                                         //  84:Lead 4 (chiff)
    "",                                                                         //  85:Lead 5 (charang)
    "",                                                                         //  86:Lead 6 (voice)
    "",                                                                         //  87:Lead 7 (fifths)
    "",                                                                         //  88:Lead 8 (bass + lead)
    "",                                                                         //  89:Pad 1 (Fantasia)
    "",                                                                         //  90:Pad 2 (warm)
    "",                                                                         //  91:Pad 3 (polysynth)
    "",                                                                         //  92:Pad 4 (choir)
    "",                                                                         //  93:Pad 5 (bowed)
    "",                                                                         //  94:Pad 6 (metallic)
    "",                                                                         //  95:Pad 7 (halo)
    "",                                                                         //  96:Pad 8 (sweep)
    "",                                                                         //  97:FX 1 (rain)
    "",                                                                         //  98:FX 2 (soundtrack)
    "",                                                                         //  99:FX 3 (crystal)
    "",                                                                         // 100:FX 4 (atmosphere)
    "",                                                                         // 101:FX 5 (brightness)
    "",                                                                         // 102:FX 6 (goblins)
    "",                                                                         // 103:FX 7 (echoes)
    "",                                                                         // 104:FX 8 (sci-fi)
    "",                                                                         // 105:Sitar
    "15,15,15,0,0,0,0,0,0,0,0,0,0,0,0,0",                                       // 106:Banjo
    "",                                                                         // 107:Shamisen
    "",                                                                         // 108:Koto
    "",                                                                         // 109:Kalimba
    "",                                                                         // 110:Bagpipe
    "",                                                                         // 111:Fiddle
    "",                                                                         // 112:Shanai
    "",                                                                         // 113:Tinkle Bell
    "",                                                                         // 114:Agogo
    "",                                                                         // 115:Steel Drums
    "",                                                                         // 116:Woodblock
    "",                                                                         // 117:Taiko Drum
    "",                                                                         // 118:Melodic Tom
    "",                                                                         // 119:Synth Drum
    "",                                                                         // 120:Reverse Cymbal
    "",                                                                         // 121:Guitar Fret Noise
    "",                                                                         // 122:Breath Noise
    "",                                                                         // 123:Seashore
    "",                                                                         // 124:Bird Tweet
    "",                                                                         // 125:Telephone Ring
    "",                                                                         // 126:Helicopter
    "",                                                                         // 127:Applause
    "",                                                                         // 128:Gunshot
};
// 音色定義FDS
const array<string, 128> kToneDefFds = {
    " 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,"
    "38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,"
    "58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,"
    "25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4",                          //   1:Acoustic Piano
    " 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,"
    "38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,"
    "58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,"
    "25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4",                          //   2:Bright Piano
    " 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,"
    "38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,"
    "58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,"
    "25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4",                          //   3:Electric Grand Piano
    " 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,"
    "38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,"
    "58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,"
    "25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4",                          //   4:Honky-tonk Piano
    " 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,"
    "38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,"
    "58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,"
    "25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4",                          //   5:Electric Piano
    " 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,"
    "38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,"
    "58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,"
    "25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4",                          //   6:Electric Piano 2
    " 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,"
    "38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,"
    "58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,"
    "25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4",                          //   7:Harpsichord
    " 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,"
    "38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,"
    "58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,"
    "25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4",                          //   8:Clavi
    "",                                                                         //   9:Celesta
    "",                                                                         //  10:Glockenspiel
    "",                                                                         //  11:Musical box
    "",                                                                         //  12:Vibraphone
    "",                                                                         //  13:Marimba
    "",                                                                         //  14:Xylophone
    "",                                                                         //  15:Tubular Bell
    "",                                                                         //  16:Dulcimer
    "",                                                                         //  17:Drawbar Organ
    "",                                                                         //  18:Percussive Organ
    "",                                                                         //  19:Rock Organ
    "",                                                                         //  20:Church organ
    "",                                                                         //  21:Reed organ
    "",                                                                         //  22:Accordion
    "",                                                                         //  23:Harmonica
    "",                                                                         //  24:Tango Accordion
    "",                                                                         //  25:Acoustic Guitar (nylon)
    "",                                                                         //  26:Acoustic Guitar (steel)
    "",                                                                         //  27:Electric Guitar (jazz)
    "",                                                                         //  28:Electric Guitar (clean)
    "",                                                                         //  29:Electric Guitar (muted)
    "",                                                                         //  30:Overdriven Guitar
    "",                                                                         //  31:Distortion Guitar
    "",                                                                         //  32:Guitar harmonics
    "",                                                                         //  33:Acoustic Bass
    "",                                                                         //  34:Electric Bass (finger)
    "",                                                                         //  35:Electric Bass (pick)
    "",                                                                         //  36:Fretless Bass
    "",                                                                         //  37:Slap Bass 1
    "",                                                                         //  38:Slap Bass 2
    "",                                                                         //  39:Synth Bass 1
    "",                                                                         //  40:Synth Bass 2
    "",                                                                         //  41:Violin
    "",                                                                         //  42:Viola
    "",                                                                         //  43:Cello
    "",                                                                         //  44:Double bass
    "",                                                                         //  45:Tremolo Strings
    "",                                                                         //  46:Pizzicato Strings
    "",                                                                         //  47:Orchestral Harp
    "",                                                                         //  48:Timpani
    "",                                                                         //  49:String Ensemble 1
    "",                                                                         //  50:String Ensemble 2
    "",                                                                         //  51:Synth Strings 1
    "",                                                                         //  52:Synth Strings 2
    "",                                                                         //  53:Voice Aahs
    "",                                                                         //  54:Voice Oohs
    "",                                                                         //  55:Synth Voice
    "",                                                                         //  56:Orchestra Hit
    "",                                                                         //  57:Trumpet
    "",                                                                         //  58:Trombone
    "",                                                                         //  59:Tuba
    "",                                                                         //  60:Muted Trumpet
    "",                                                                         //  61:French horn
    "",                                                                         //  62:Brass Section
    "",                                                                         //  63:Synth Brass 1
    "",                                                                         //  64:Synth Brass 2
    "",                                                                         //  65:Soprano Sax
    "",                                                                         //  66:Alto Sax
    "",                                                                         //  67:Tenor Sax
    "",                                                                         //  68:Baritone Sax
    "",                                                                         //  69:Oboe
    "",                                                                         //  70:English Horn
    "",                                                                         //  71:Bassoon
    "",                                                                         //  72:Clarinet
    "",                                                                         //  73:Piccolo
    "",                                                                         //  74:Flute
    "",                                                                         //  75:Recorder
    "",                                                                         //  76:Pan Flute
    "",                                                                         //  77:Blown Bottle
    "",                                                                         //  78:Shakuhachi
    "",                                                                         //  79:Whistle
    "",                                                                         //  80:Ocarina
    "",                                                                         //  81:Lead 1 (square)
    "",                                                                         //  82:Lead 2 (sawtooth)
    "",                                                                         //  83:Lead 3 (calliope)
    "",                                                                         //  84:Lead 4 (chiff)
    "",                                                                         //  85:Lead 5 (charang)
    "",                                                                         //  86:Lead 6 (voice)
    "",                                                                         //  87:Lead 7 (fifths)
    "",                                                                         //  88:Lead 8 (bass + lead)
    "",                                                                         //  89:Pad 1 (Fantasia)
    "",                                                                         //  90:Pad 2 (warm)
    "",                                                                         //  91:Pad 3 (polysynth)
    "",                                                                         //  92:Pad 4 (choir)
    "",                                                                         //  93:Pad 5 (bowed)
    "",                                                                         //  94:Pad 6 (metallic)
    "",                                                                         //  95:Pad 7 (halo)
    "",                                                                         //  96:Pad 8 (sweep)
    "",                                                                         //  97:FX 1 (rain)
    "",                                                                         //  98:FX 2 (soundtrack)
    "",                                                                         //  99:FX 3 (crystal)
    "",                                                                         // 100:FX 4 (atmosphere)
    "",                                                                         // 101:FX 5 (brightness)
    "",                                                                         // 102:FX 6 (goblins)
    "",                                                                         // 103:FX 7 (echoes)
    "",                                                                         // 104:FX 8 (sci-fi)
    "",                                                                         // 105:Sitar
    "",                                                                         // 106:Banjo
    "",                                                                         // 107:Shamisen
    "",                                                                         // 108:Koto
    "",                                                                         // 109:Kalimba
    "",                                                                         // 110:Bagpipe
    "",                                                                         // 111:Fiddle
    "",                                                                         // 112:Shanai
    "",                                                                         // 113:Tinkle Bell
    "",                                                                         // 114:Agogo
    "",                                                                         // 115:Steel Drums
    "",                                                                         // 116:Woodblock
    "",                                                                         // 117:Taiko Drum
    "",                                                                         // 118:Melodic Tom
    "",                                                                         // 119:Synth Drum
    "",                                                                         // 120:Reverse Cymbal
    "",                                                                         // 121:Guitar Fret Noise
    "",                                                                         // 122:Breath Noise
    "",                                                                         // 123:Seashore
    "",                                                                         // 124:Bird Tweet
    "",                                                                         // 125:Telephone Ring
    "",                                                                         // 126:Helicopter
    "",                                                                         // 127:Applause
    "",                                                                         // 128:Gunshot
};

// ====音色別音量エンヴェロープ定義=====
// 音色別音量エンヴェロープ定義ノイズ
const array<string, 128> kVolumeDefNoise = {
    "",                                                                         //   0:
    "",                                                                         //   1:
    "",                                                                         //   2:
    "",                                                                         //   3:
    "",                                                                         //   4:
    "",                                                                         //   5:
    "",                                                                         //   6:
    "",                                                                         //   7:
    "",                                                                         //   8:
    "",                                                                         //   9:
    "",                                                                         //  10:
    "",                                                                         //  11:
    "",                                                                         //  12:
    "",                                                                         //  13:
    "",                                                                         //  14:
    "",                                                                         //  15:
    "",                                                                         //  16:
    "",                                                                         //  17:
    "",                                                                         //  18:
    "",                                                                         //  19:
    "",                                                                         //  20:
    "",                                                                         //  21:
    "",                                                                         //  22:
    "",                                                                         //  23:
    "",                                                                         //  24:
    "",                                                                         //  25:Snare Roll
    "",                                                                         //  26:Finger Snap
    "",                                                                         //  27:High Q
    "",                                                                         //  28:Slap
    "",                                                                         //  29:Scratch Push
    "",                                                                         //  30:Scratch Pull
    "15,14,13,12,11,10,0",                                                      //  31:Sticks
    "15,14,13,12,11,10,0",                                                      //  32:Square Click
    "",                                                                         //  33:Metronome Click
    "",                                                                         //  34:Metronome Bell
    "",                                                                         //  35:Bass Drum 2
    "",                                                                         //  36:Bass Drum 1
    "",                                                                         //  37:Side Stick
    "",                                                                         //  38:Snare Drum 1
    "15,14,13,12,11,10,0",                                                      //  39:Hand Clap
    "",                                                                         //  40:Snare Drum 2
    "",                                                                         //  41:Low Tom 2
    "12,9,6,3,0",                                                               //  42:Closed Hi-hat
    "",                                                                         //  43:Low Tom 1
    "12,11,10,9,8,7,6,5,4,3,2,1,0",                                             //  44:Pedal Hi-hat
    "",                                                                         //  45:Mid Tom 2
    "15,15,14,13,13,12,11,11,10,9,9,8,7,7,6,5,5,4,3,3,2,1,1,0",                 //  46:Open Hi-hat
    "",                                                                         //  47:Mid Tom 1
    "",                                                                         //  48:High Tom 2
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,"
    "5,4,4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                //  49:Crash Cymbal 1
    "",                                                                         //  50:High Tom 1
    "15,15,14,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,"
    "5,5,5,5,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1,0",                                  //  51:Ride Cymbal 1
    "15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0",                                    //  52:Chinese Cymbal
    "12,11,10,9,8,7,6,5,4,3,2,1,0",                                             //  53:Ride Bell
    "15,13,11,9,7,5,3,1,0",                                                     //  54:Tambourine
    "15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0",                                    //  55:Splash Cymbal
    "",                                                                         //  56:Cowbell
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,"
    "5,4,4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                //  57:Crash Cymbal 2
    "15,15,14,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,"
    "5,5,5,5,4,4,4,3,3,3,3,0",                                                  //  58:Vibra Slap
    "15,15,14,14,13,13,12,12,11,11,10,10,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,"
    "1,0",                                                                      //  59:Ride Cymbal 2
    "",                                                                         //  60:High Bongo
    "",                                                                         //  61:Low Bongo
    "",                                                                         //  62:Mute High Conga
    "",                                                                         //  63:Open High Conga
    "",                                                                         //  64:Low Conga
    "",                                                                         //  65:High Timbale
    "",                                                                         //  66:Low Timbale
    "",                                                                         //  67:High Agogo
    "",                                                                         //  68:Low Agogo
    "",                                                                         //  69:Cabasa
    "15,14,13,12,11,10,0",                                                      //  70:Maracas
    "",                                                                         //  71:Short Whistle
    "",                                                                         //  72:Long Whistle
    "",                                                                         //  73:Short Guiro
    "",                                                                         //  74:Long Guiro
    "",                                                                         //  75:Claves
    "",                                                                         //  76:High Wood Block
    "",                                                                         //  77:Low Wood Block
    "",                                                                         //  78:Mute Cuica
    "",                                                                         //  79:Open Cuica
    "",                                                                         //  70:Mute Triangle
    "",                                                                         //  81:Open Triangle
    "10,11,14,15,13,0",                                                         //  82:Shaker
    "",                                                                         //  83:Jingle Bell
    "",                                                                         //  84:Bell Tree
    "",                                                                         //  85:Castanets
    "",                                                                         //  86:Mute Surdo
    "",                                                                         //  87:Open Surdo
    "",                                                                         //  88:Low Whistle
    "",                                                                         //  89:Mute Cuica
    "",                                                                         //  90:Open Cuica
    "",                                                                         //  91:Mute Triangle
    "",                                                                         //  92:Open Triangle
    "",                                                                         //  93:Short Guiro
    "",                                                                         //  94:Long Guiro
    "",                                                                         //  95:Cabasa Up
    "",                                                                         //  96:Cabasa Down
    "",                                                                         //  97:Claves
    "",                                                                         //  98:High Wood Block
    "",                                                                         //  99:Low Wood Block
    "",                                                                         // 100:
    "",                                                                         // 101:
    "",                                                                         // 102:
    "",                                                                         // 103:
    "",                                                                         // 104:
    "",                                                                         // 105:
    "",                                                                         // 106:
    "",                                                                         // 107:
    "",                                                                         // 108:
    "",                                                                         // 109:
    "",                                                                         // 110:
    "",                                                                         // 111:
    "",                                                                         // 112:
    "",                                                                         // 113:
    "",                                                                         // 114:
    "",                                                                         // 115:
    "",                                                                         // 116:
    "",                                                                         // 117:
    "",                                                                         // 118:
    "",                                                                         // 119:
    "",                                                                         // 120:
    "",                                                                         // 121:
    "",                                                                         // 122:
    "",                                                                         // 123:
    "",                                                                         // 124:
    "",                                                                         // 125:
    "",                                                                         // 126:
    "",                                                                         // 127:
};
// 音色別音量エンヴェロープ定義共通
const array<string, 128> kVolumeDefCommon = {
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,5,4,"
    "4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                    //   1:Acoustic Piano
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,5,4,"
    "4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                    //   2:Bright Piano
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,5,4,"
    "4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                    //   3:Electric Grand Piano
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,5,4,"
    "4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                    //   4:Honky-tonk Piano
    "",                                                                         //   5:Electric Piano
    "",                                                                         //   6:Electric Piano 2
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,5,4,"
    "4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                    //   7:Harpsichord
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,5,4,"
    "4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                    //   8:Clavi
    "",                                                                         //   9:Celesta
    "",                                                                         //  10:Glockenspiel
    "",                                                                         //  11:Musical box
    "",                                                                         //  12:Vibraphone
    "",                                                                         //  13:Marimba
    "",                                                                         //  14:Xylophone
    "",                                                                         //  15:Tubular Bell
    "",                                                                         //  16:Dulcimer
    "",                                                                         //  17:Drawbar Organ
    "",                                                                         //  18:Percussive Organ
    "",                                                                         //  19:Rock Organ
    "",                                                                         //  20:Church organ
    "",                                                                         //  21:Reed organ
    "",                                                                         //  22:Accordion
    "",                                                                         //  23:Harmonica
    "",                                                                         //  24:Tango Accordion
    "",                                                                         //  25:Acoustic Guitar (nylon)
    "",                                                                         //  26:Acoustic Guitar (steel)
    "",                                                                         //  27:Electric Guitar (jazz)
    "",                                                                         //  28:Electric Guitar (clean)
    "",                                                                         //  29:Electric Guitar (muted)
    "",                                                                         //  30:Overdriven Guitar
    "",                                                                         //  31:Distortion Guitar
    "",                                                                         //  32:Guitar harmonics
    "",                                                                         //  33:Acoustic Bass
    "",                                                                         //  34:Electric Bass (finger)
    "",                                                                         //  35:Electric Bass (pick)
    "",                                                                         //  36:Fretless Bass
    "15,14,13,12,11,10,9,8,7,6,5,4,3",                                          //  37:Slap Bass 1
    "15,14,13,12,11,10,9,8,7,6,5,4,3",                                          //  38:Slap Bass 2
    "",                                                                         //  39:Synth Bass 1
    "",                                                                         //  40:Synth Bass 2
    "11,12,14,15",                                                              //  41:Violin
    "11,12,14,15",                                                              //  42:Viola
    "11,12,14,15",                                                              //  43:Cello
    "11,12,14,15",                                                              //  44:Double bass
    "11,12,14,15",                                                              //  45:Tremolo Strings
    "15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0",                                    //  46:Pizzicato Strings
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,5,4,"
    "4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                    //  47:Orchestral Harp
    "15,14,13,13,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,7,6,6,6,6,5,5,5,5,4,"
    "4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0",                                    //  48:Timpani
    "11,12,14,15",                                                              //  49:String Ensemble 1
    "5,6,7,8,9,10,11,12,13,14,15",                                              //  50:String Ensemble 2
    "11,12,14,15",                                                              //  51:Synth Strings 1
    "11,12,14,15",                                                              //  52:Synth Strings 2
    "",                                                                         //  53:Voice Aahs
    "",                                                                         //  54:Voice Oohs
    "",                                                                         //  55:Synth Voice
    "15,15,15,15,15,15,15,15,15,15,15,15,14,14,14,14,14,14,13,13,13,13,13,12,"
    "12,12,12,12,11,11,11,11,10,10,10,10,9,9,9,9,8,8,8,8,7,7,7,7,6,6,6,6,5,5,5,"
    "5,4,4,4,4,3",                                                              //  56:Orchestra Hit
    "",                                                                         //  57:Trumpet
    "",                                                                         //  58:Trombone
    "",                                                                         //  59:Tuba
    "",                                                                         //  60:Muted Trumpet
    "",                                                                         //  61:French horn
    "11,13,15",                                                                 //  62:Brass Section
    "11,13,15",                                                                 //  63:Synth Brass 1
    "11,13,15",                                                                 //  64:Synth Brass 2
    "11,13,15",                                                                 //  65:Soprano Sax
    "11,13,15",                                                                 //  66:Alto Sax
    "11,13,15",                                                                 //  67:Tenor Sax
    "11,13,15",                                                                 //  68:Baritone Sax
    "11,13,15",                                                                 //  69:Oboe
    "11,13,15",                                                                 //  70:English Horn
    "11,13,15",                                                                 //  71:Bassoon
    "11,13,15",                                                                 //  72:Clarinet
    "11,13,15",                                                                 //  73:Piccolo
    "11,13,15",                                                                 //  74:Flute
    "11,13,15",                                                                 //  75:Recorder
    "11,13,15",                                                                 //  76:Pan Flute
    "11,13,15",                                                                 //  77:Blown Bottle
    "11,13,15",                                                                 //  78:Shakuhachi
    "11,13,15",                                                                 //  79:Whistle
    "15",                                                                       //  80:Ocarina
    "15",                                                                       //  81:Lead 1 (square)
    "15",                                                                       //  82:Lead 2 (sawtooth)
    "",                                                                         //  83:Lead 3 (calliope)
    "",                                                                         //  84:Lead 4 (chiff)
    "",                                                                         //  85:Lead 5 (charang)
    "11,13,15",                                                                 //  86:Lead 6 (voice)
    "",                                                                         //  87:Lead 7 (fifths)
    "",                                                                         //  88:Lead 8 (bass + lead)
    "",                                                                         //  89:Pad 1 (Fantasia)
    "",                                                                         //  90:Pad 2 (warm)
    "",                                                                         //  91:Pad 3 (polysynth)
    "",                                                                         //  92:Pad 4 (choir)
    "",                                                                         //  93:Pad 5 (bowed)
    "",                                                                         //  94:Pad 6 (metallic)
    "",                                                                         //  95:Pad 7 (halo)
    "",                                                                         //  96:Pad 8 (sweep)
    "",                                                                         //  97:FX 1 (rain)
    "",                                                                         //  98:FX 2 (soundtrack)
    "",                                                                         //  99:FX 3 (crystal)
    "",                                                                         // 100:FX 4 (atmosphere)
    "",                                                                         // 101:FX 5 (brightness)
    "",                                                                         // 102:FX 6 (goblins)
    "",                                                                         // 103:FX 7 (echoes)
    "",                                                                         // 104:FX 8 (sci-fi)
    "",                                                                         // 105:Sitar
    "",                                                                         // 106:Banjo
    "",                                                                         // 107:Shamisen
    "",                                                                         // 108:Koto
    "",                                                                         // 109:Kalimba
    "",                                                                         // 110:Bagpipe
    "",                                                                         // 111:Fiddle
    "",                                                                         // 112:Shanai
    "",                                                                         // 113:Tinkle Bell
    "",                                                                         // 114:Agogo
    "",                                                                         // 115:Steel Drums
    "",                                                                         // 116:Woodblock
    "",                                                                         // 117:Taiko Drum
    "",                                                                         // 118:Melodic Tom
    "",                                                                         // 119:Synth Drum
    "1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,"
    "10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15",           // 120:Reverse Cymbal
    "",                                                                         // 121:Guitar Fret Noise
    "",                                                                         // 122:Breath Noise
    "",                                                                         // 123:Seashore
    "",                                                                         // 124:Bird Tweet
    "",                                                                         // 125:Telephone Ring
    "",                                                                         // 126:Helicopter
    "",                                                                         // 127:Applause
    "",                                                                         // 128:Gunshot
};
// 音色別音量エンヴェロープ定義VRC7(プリセット音色用)
const array<string, 128> kVolumeDefVrc7 = {
    "",                                                                         //   1:Acoustic Piano
    "",                                                                         //   2:Bright Piano
    "",                                                                         //   3:Electric Grand Piano
    "",                                                                         //   4:Honky-tonk Piano
    "",                                                                         //   5:Electric Piano
    "15,14,13,12",                                                              //   6:Electric Piano 2
    "",                                                                         //   7:Harpsichord
    "",                                                                         //   8:Clavi
    "15,14,13",                                                                 //   9:Celesta
    "15,14,13",                                                                 //  10:Glockenspiel
    "15,14,13",                                                                 //  11:Musical box
    "15,14,14,13,12,11,10,9",                                                   //  12:Vibraphone
    "15,14,13",                                                                 //  13:Marimba
    "15,14,13",                                                                 //  14:Xylophone
    "",                                                                         //  15:Tubular Bell
    "",                                                                         //  16:Dulcimer
    "",                                                                         //  17:Drawbar Organ
    "",                                                                         //  18:Percussive Organ
    "",                                                                         //  19:Rock Organ
    "",                                                                         //  20:Church organ
    "",                                                                         //  21:Reed organ
    "",                                                                         //  22:Accordion
    "",                                                                         //  23:Harmonica
    "",                                                                         //  24:Tango Accordion
    "15,14,13",                                                                 //  25:Acoustic Guitar (nylon)
    "15,14,13",                                                                 //  26:Acoustic Guitar (steel)
    "15,14,13",                                                                 //  27:Electric Guitar (jazz)
    "15,14,13",                                                                 //  28:Electric Guitar (clean)
    "15,14,13",                                                                 //  29:Electric Guitar (muted)
    "",                                                                         //  30:Overdriven Guitar
    "",                                                                         //  31:Distortion Guitar
    "",                                                                         //  32:Guitar harmonics
    "15,13,12",                                                                 //  33:Acoustic Bass
    "",                                                                         //  34:Electric Bass (finger)
    "15,14,13",                                                                 //  35:Electric Bass (pick)
    "",                                                                         //  36:Fretless Bass
    "15,14,13",                                                                 //  37:Slap Bass 1
    "15,14,13",                                                                 //  38:Slap Bass 2
    "",                                                                         //  39:Synth Bass 1
    "",                                                                         //  40:Synth Bass 2
    "",                                                                         //  41:Violin
    "",                                                                         //  42:Viola
    "",                                                                         //  43:Cello
    "",                                                                         //  44:Double bass
    "",                                                                         //  45:Tremolo Strings
    "",                                                                         //  46:Pizzicato Strings
    "15,13",                                                                    //  47:Orchestral Harp
    "",                                                                         //  48:Timpani
    "",                                                                         //  49:String Ensemble 1
    "5,6,7,8,9,10,11,12,13,14,15",                                              //  50:String Ensemble 2
    "",                                                                         //  51:Synth Strings 1
    "",                                                                         //  52:Synth Strings 2
    "",                                                                         //  53:Voice Aahs
    "",                                                                         //  54:Voice Oohs
    "",                                                                         //  55:Synth Voice
    "15,15,15,15,15,15,15,15,15,15,15,15,14,14,14,14,14,14,13,13,13,13,13,12,"
    "12,12,12,12,11,11,11,11,10,10,10,10,9,9,9,9,8,8,8,8,7,7,7,7,6,6,6,6,5,5,5,"
    "5,4,4,4,4,3",                                                              //  56:Orchestra Hit
    "",                                                                         //  57:Trumpet
    "",                                                                         //  58:Trombone
    "",                                                                         //  59:Tuba
    "",                                                                         //  60:Muted Trumpet
    "",                                                                         //  61:French horn
    "",                                                                         //  62:Brass Section
    "",                                                                         //  63:Synth Brass 1
    "",                                                                         //  64:Synth Brass 2
    "",                                                                         //  65:Soprano Sax
    "",                                                                         //  66:Alto Sax
    "",                                                                         //  67:Tenor Sax
    "",                                                                         //  68:Baritone Sax
    "",                                                                         //  69:Oboe
    "",                                                                         //  70:English Horn
    "",                                                                         //  71:Bassoon
    "",                                                                         //  72:Clarinet
    "",                                                                         //  73:Piccolo
    "",                                                                         //  74:Flute
    "",                                                                         //  75:Recorder
    "",                                                                         //  76:Pan Flute
    "",                                                                         //  77:Blown Bottle
    "",                                                                         //  78:Shakuhachi
    "",                                                                         //  79:Whistle
    "",                                                                         //  80:Ocarina
    "",                                                                         //  81:Lead 1 (square)
    "",                                                                         //  82:Lead 2 (sawtooth)
    "",                                                                         //  83:Lead 3 (calliope)
    "",                                                                         //  84:Lead 4 (chiff)
    "",                                                                         //  85:Lead 5 (charang)
    "",                                                                         //  86:Lead 6 (voice)
    "",                                                                         //  87:Lead 7 (fifths)
    "",                                                                         //  88:Lead 8 (bass + lead)
    "",                                                                         //  89:Pad 1 (Fantasia)
    "",                                                                         //  90:Pad 2 (warm)
    "",                                                                         //  91:Pad 3 (polysynth)
    "",                                                                         //  92:Pad 4 (choir)
    "",                                                                         //  93:Pad 5 (bowed)
    "",                                                                         //  94:Pad 6 (metallic)
    "",                                                                         //  95:Pad 7 (halo)
    "",                                                                         //  96:Pad 8 (sweep)
    "",                                                                         //  97:FX 1 (rain)
    "",                                                                         //  98:FX 2 (soundtrack)
    "",                                                                         //  99:FX 3 (crystal)
    "",                                                                         // 100:FX 4 (atmosphere)
    "",                                                                         // 101:FX 5 (brightness)
    "",                                                                         // 102:FX 6 (goblins)
    "",                                                                         // 103:FX 7 (echoes)
    "",                                                                         // 104:FX 8 (sci-fi)
    "",                                                                         // 105:Sitar
    "",                                                                         // 106:Banjo
    "",                                                                         // 107:Shamisen
    "",                                                                         // 108:Koto
    "",                                                                         // 109:Kalimba
    "",                                                                         // 110:Bagpipe
    "",                                                                         // 111:Fiddle
    "",                                                                         // 112:Shanai
    "",                                                                         // 113:Tinkle Bell
    "",                                                                         // 114:Agogo
    "",                                                                         // 115:Steel Drums
    "",                                                                         // 116:Woodblock
    "",                                                                         // 117:Taiko Drum
    "",                                                                         // 118:Melodic Tom
    "",                                                                         // 119:Synth Drum
    "1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,"
    "10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15",           // 120:Reverse Cymbal
    "",                                                                         // 121:Guitar Fret Noise
    "",                                                                         // 122:Breath Noise
    "",                                                                         // 123:Seashore
    "",                                                                         // 124:Bird Tweet
    "",                                                                         // 125:Telephone Ring
    "",                                                                         // 126:Helicopter
    "",                                                                         // 127:Applause
    "",                                                                         // 128:Gunshot
};
