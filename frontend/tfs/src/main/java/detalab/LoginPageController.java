package detalab;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;
import java.util.Optional;

import javafx.fxml.FXML;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.PasswordField;
import javafx.scene.control.TextField;
import javafx.scene.control.Alert;
import javafx.scene.control.ButtonType;


public class LoginPageController {

    protected static Optional<ButtonType> showAlert(Alert.AlertType alertType, String title, String header, String content){
        Alert alert = new Alert(alertType);
        alert.setTitle(title);
        alert.setHeaderText(header);
        alert.setContentText(content);
        return alert.showAndWait();
    }

    @FXML
    private void switchToRegister() throws IOException {
        App.setRoot("register");
    }

    @FXML
    TextField emailField;

    @FXML
    PasswordField passwordField;

    @FXML
    private void Login() throws IOException {

        String username = usernameField.getText();
        String password = passwordField.getText();

        try {
            URL url = new URL("http://localhost:8080/login");
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();

            // Configurazione della connessione
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Content-Type", "application/json");
            conn.setDoOutput(true);

            // Corpo della richiesta JSON
            String jsonInput = "{"
                    + "\"username\":\"" + username + "\","
                    + "\"password\":\"" + password + "\""
                    + "}";


            // Scrittura del body nella richiesta
            try (OutputStream os = conn.getOutputStream()) {
                byte[] input = jsonInput.getBytes("utf-8");
                os.write(input, 0, input.length);
            }

            // Lettura della risposta
            int statusCode = conn.getResponseCode();
            BufferedReader br = new BufferedReader(new InputStreamReader(
                    (statusCode == 200) ? conn.getInputStream() : conn.getErrorStream(),
                    "utf-8"));

            StringBuilder response = new StringBuilder();
            String responseLine;
            while ((responseLine = br.readLine()) != null) {
                response.append(responseLine.trim());
            }

            // Controllo della risposta
            System.out.println("HTTP Status: " + statusCode);
            System.out.println("Risposta: " + response.toString());

            if (statusCode == 200) {
                System.out.println("Login riuscito!");
            } else if (statusCode == 401) {
                System.out.println("Credenziali errate!");
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Credenziali errate!");
            } else {
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
            }

        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

    }

}