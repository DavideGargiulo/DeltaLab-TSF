package detalab.DTO;

import java.util.*;

public class LanguageSet {
  
    private static final Map<String, String> languages = new LinkedHashMap<>();
    static {
        languages.put("it", "Italiano");
        languages.put("en", "English");
        languages.put("fr", "Français");
        languages.put("de", "Deutsch");
        languages.put("es", "Español");
        languages.put("pt", "Português");
        languages.put("nl", "Nederlands");
        languages.put("sv", "Svenska");
        languages.put("no", "Norsk");
        languages.put("da", "Dansk");
        languages.put("fi", "Suomi");
        languages.put("pl", "Polski");
        languages.put("cs", "Čeština");
        languages.put("sk", "Slovenčina");
        languages.put("hu", "Magyar");
        languages.put("ro", "Română");
        languages.put("bg", "Български");
        languages.put("el", "Ελληνικά");
        languages.put("hr", "Hrvatski");
        languages.put("sl", "Slovenščina");
        languages.put("lt", "Lietuvių");
        languages.put("lv", "Latviešu");
        languages.put("et", "Eesti");
        languages.put("ja", "日本語");
        languages.put("ru", "Русский");
        languages.put("zh", "中文");
    }

    // Restituisce la lista di tutti i nomi
    public static List<String> getLanguageNames() {
        return new ArrayList<>(languages.values());
    }

    // Restituisce la lista di tutti i codici
    public static List<String> getLanguageCodes() {
        return new ArrayList<>(languages.keySet());
    }

    // Converte da codice a nome
    public static String getNameFromCode(String code) {
        return languages.get(code);
    }

    // Converte da nome a codice
    public static String getCodeFromName(String name) {
        return languages.entrySet()
                .stream()
                .filter(e -> e.getValue().equalsIgnoreCase(name))
                .map(Map.Entry::getKey)
                .findFirst()
                .orElse(null);
    }

}
