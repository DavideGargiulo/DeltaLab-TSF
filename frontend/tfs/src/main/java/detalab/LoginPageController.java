package detalab;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;

import javafx.fxml.FXML;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.PasswordField;
import javafx.scene.control.TextField;
import javafx.scene.control.Alert;


public class LoginPageController extends GeneralPageController {

    @FXML
    private void switchToRegister() throws IOException {
        App.setRoot("register");
    }

    @FXML
    TextField usernameField;

    @FXML
    PasswordField passwordField;

    @FXML
    private void Login() throws IOException {

        String username = usernameField.getText();
        String password = passwordField.getText();

        try {

            // Corpo della richiesta JSON
            String jsonInput = "{"
                    + "\"username\":\"" + username + "\","
                    + "\"password\":\"" + password + "\""
                    + "}";

            HttpURLConnection conn = makeRequest("login", "POST", jsonInput);

            // Lettura della risposta
            int statusCode = conn.getResponseCode();

            String response = readAnswer(conn, statusCode);

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