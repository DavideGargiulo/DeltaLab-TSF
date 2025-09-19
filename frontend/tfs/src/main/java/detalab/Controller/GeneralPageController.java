package detalab;

import java.io.IOException;
import java.net.URL;
import java.util.Optional;
import java.net.HttpURLConnection;
import java.io.OutputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import org.json.*;
import detalab.DTO.Response;

import javafx.fxml.FXML;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.ButtonType;
import javafx.scene.control.Alert;
import javafx.scene.Scene;
import javafx.stage.Modality;
import javafx.stage.Stage;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.layout.BorderPane;


public abstract class GeneralPageController {

    protected static Optional<ButtonType> showAlert(Alert.AlertType alertType, String title, String header, String content){
        Alert alert = new Alert(alertType);
        alert.setTitle(title);
        alert.setHeaderText(header);
        alert.setContentText(content);
        return alert.showAndWait();
    }

    private String url = "http://localhost:8080/";

    protected Response makeRequest(String endpoint, String method, int expectedStatus) throws IOException {
        return makeRequest(endpoint, method, "", expectedStatus);
    }

    protected Response makeRequest(String endpoint, String method, String body, int expectedStatus) throws IOException {
        URL url = new URL(this.url + endpoint);
        HttpURLConnection conn = (HttpURLConnection) url.openConnection();

        // Configurazione della connessione
        conn.setRequestMethod(method);
        
        if (method.equals("POST") || method.equals("PUT")) {

            conn.setRequestProperty("Content-Type", "application/json");
            conn.setDoOutput(true);

            // Scrittura del body nella richiesta
            try (OutputStream os = conn.getOutputStream()) {
                byte[] input = body.getBytes("utf-8");
                os.write(input, 0, input.length);
            }
        }
        
        // Leggi lo status code
        int statusCode = conn.getResponseCode();

        BufferedReader br = new BufferedReader(new InputStreamReader(
                    (statusCode == expectedStatus) ? conn.getInputStream() : conn.getErrorStream(),
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

        return new Response(json.getBoolean("result"), statusCode, json.getString("message"), json.optString("content", null));
        
    }

}