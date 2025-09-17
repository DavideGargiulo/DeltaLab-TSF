package detalab;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;
import main.java.detalab.DTO.Response;
import org.json.*;

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
    TextField emailField;

    @FXML
    PasswordField passwordField;

    @FXML
    private void Login() throws IOException {

        String email = emailField.getText();
        String password = passwordField.getText();

        try {

            // Corpo della richiesta JSON
            String jsonInput = "{"
                    + "\"email\":\"" + email + "\","
                    + "\"password\":\"" + password + "\""
                    + "}";

            Response response = makeRequest("auth/login", "POST", jsonInput, 200);

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());

            if (response.getStatus() == 200) {
                App.setRoot("main");
            } else {
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
            }

        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

    }

}