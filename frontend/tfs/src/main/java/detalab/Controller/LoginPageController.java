package detalab.Controller;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;
import detalab.DTO.Response;
import detalab.DTO.User;
import detalab.DTO.LoggedUser;
import detalab.App;
import org.json.*;

import javafx.fxml.FXML;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.layout.BorderPane;
import javafx.scene.control.PasswordField;
import javafx.scene.control.TextField;
import javafx.stage.Stage;
import javafx.scene.Scene;
import javafx.scene.control.Alert;


public class LoginPageController extends GeneralPageController {

    @FXML
    private void switchToSignup() throws IOException {
        App.setRoot("signup");
    }

    @FXML
    private BorderPane root;

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
            JSONObject requestBody = new JSONObject();
            requestBody.put("email", email);
            requestBody.put("password", password);

            Response response = makeRequest("auth/login", "POST", requestBody.toString(), 200);

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
            System.out.println("content: " + response.getContent() + "\n");

            if (response.getStatus() == 200) {
                JSONObject json = new JSONObject(response.getContent());
                LoggedUser.getInstance(new User(json.getInt("id"), json.getString("email"), json.getString("username"), json.getString("lingua")));
                Scene scene = root.getScene();
                Stage stage = (Stage) scene.getWindow();
                // Imposta le dimensioni della Scene
                scene.getWindow().setWidth(1200 + (stage.getWidth() - scene.getWidth()));
                scene.getWindow().setHeight(700 + (stage.getHeight() - scene.getHeight()));
                stage.centerOnScreen();
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