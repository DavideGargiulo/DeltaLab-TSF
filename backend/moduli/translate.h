#ifndef TRANSLATE_H
#define TRANSLATE_H

// Funzione principale per tradurre un testo
// text   = stringa da tradurre
// source = lingua sorgente (es. "en")
// target = lingua target (es. "it")
// Ritorna stringa allocata con strdup, contenente solo il testo tradotto
// Devi fare free() dopo l’uso
char* translate_text(const char *text, const char *source, const char *target);

#endif
