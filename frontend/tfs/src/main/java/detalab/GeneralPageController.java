package detalab;

import java.io.IOException;
import java.net.URL;
import java.util.Optional;

import javafx.fxml.FXML;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.ButtonType;
import javafx.scene.control.Alert;

public abstract class GeneralPageController {

    protected static Optional<ButtonType> showAlert(Alert.AlertType alertType, String title, String header, String content){
        Alert alert = new Alert(alertType);
        alert.setTitle(title);
        alert.setHeaderText(header);
        alert.setContentText(content);
        return alert.showAndWait();
    }

    private String url = "http://localhost:8080/";

    protected HttpURLConnection makeRequest(String endpoint, String method, String body) throws IOException {
        URL url = new URL(this.url + endpoint);
        HttpURLConnection conn = (HttpURLConnection) url.openConnection();

        // Configurazione della connessione
        conn.setRequestMethod(method);
        conn.setRequestProperty("Content-Type", "application/json");
        conn.setDoOutput(true);
        
        // Scrittura del body nella richiesta
        try (OutputStream os = conn.getOutputStream()) {
            byte[] input = body.getBytes("utf-8");
            os.write(input, 0, input.length);
        }
        
        return conn;
    }

    protected String readAnswer(HttpURLConnection conn, int statusCode) throws IOException {
        BufferedReader br = new BufferedReader(new InputStreamReader(
                    (statusCode == 200) ? conn.getInputStream() : conn.getErrorStream(),
                    "utf-8"));

        StringBuilder response = new StringBuilder();
        String line;
        while ((line = br.readLine()) != null) {
            response.append(line.trim());
        }

        return response.toString();
    }



}