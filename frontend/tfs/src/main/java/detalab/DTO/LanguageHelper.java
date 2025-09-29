package detalab.DTO;

import java.util.*;
import detalab.Controller.GeneralPageController;
import org.json.JSONObject;
import java.net.URL;
import java.net.HttpURLConnection;
import java.io.OutputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import detalab.DTO.LoggedUser;
import java.io.IOException;

public class LanguageHelper extends GeneralPageController{

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

    public static String translate(String text) throws IOException {

        String target = LoggedUser.getInstance().getLanguage();
        String source = "en"; // Lingua di partenza (inglese)

        if (source.equals(target)) {
            return text; // Nessuna traduzione necessaria
        }

        JSONObject requestBody = new JSONObject();
        requestBody.put("q", text);
        requestBody.put("source", source);
        requestBody.put("target", target);

        URL url = new URL("http://100.90.125.46:5000/translate");
        HttpURLConnection conn = (HttpURLConnection) url.openConnection();

        // Configurazione della connessione
        conn.setRequestMethod("POST");
        conn.setRequestProperty("Content-Type", "application/json");
        conn.setDoOutput(true);

        // Scrittura del body nella richiesta
        try (OutputStream os = conn.getOutputStream()) {
            byte[] input = requestBody.toString().getBytes("utf-8");
            os.write(input, 0, input.length);
        }

        // Leggi lo status code
        int statusCode = conn.getResponseCode();

        BufferedReader br = new BufferedReader(new InputStreamReader(
                    (statusCode == 200) ? conn.getInputStream() : conn.getErrorStream(),
                    "utf-8"));

        StringBuilder response = new StringBuilder();
        String line;
        while ((line = br.readLine()) != null) {
            response.append(line.trim());
        }

        // Chiusura della connessione
        conn.disconnect();

        String responseBody = response.toString();
        JSONObject json = new JSONObject(responseBody);

        return json.getString("translatedText");
    }

}
