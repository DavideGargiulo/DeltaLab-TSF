package detalab;

import java.util.ResourceBundle;
import java.net.URL;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import detalab.DTO.LoggedUser;
import detalab.DTO.Response;
import detalab.DTO.User;
import detalab.DTO.CurrentLobby;
import detalab.DTO.Lobby;
import org.json.*;

import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.TableView;
import javafx.scene.control.TableColumn;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.Alert;
import javafx.fxml.FXML;
import javafx.scene.control.ComboBox;
import javafx.scene.control.CheckBox;
import javafx.event.ActionEvent;
import javafx.stage.Stage;
import javafx.scene.Node;


public class CreateLobbyDialogController extends GeneralPageController {

    @FXML
    private ComboBox<String> rotationBox;

    @FXML
    private CheckBox privateCheckBox;

    @FXML
    public void closeDialog() {
    Stage stage = (Stage) rotationBox.getScene().getWindow();
    stage.close();
    }

    @FXML
    private void createLobby() {

        String rotation = rotationBox.getValue();
        boolean isPrivate = privateCheckBox.isSelected();

        try {
            
            JSONObject json = new JSONObject();
            json.put("idCreator", LoggedUser.getInstance().getId());
            json.put("isPrivate", isPrivate);
            json.put("rotation", rotation.toLowerCase());

            String jsonInput = json.toString();

            System.out.println("JSON Input: " + jsonInput);

            Response response = makeRequest("lobby", "POST", jsonInput, 201);

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
            System.out.println("content: " + response.getContent() + "\n");

            if (response.getStatus() == 201) {
                showAlert(AlertType.INFORMATION, "Successo", "Lobby creata con successo.", "Ora puoi giocare!");
                setLobbySession(new JSONObject(response.getContent()).getString("id"));
                closeDialog();
            } else {
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
            }
            
        } catch (IOException e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

    }

    private void setLobbySession(String lobbyID) {

        CurrentLobby.cleanLobby();

        try {
            Response response = makeRequest("lobby/" + lobbyID, "GET", 200);

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
            System.out.println("content: " + response.getContent() + "\n");

            if (response.getStatus() == 200) {
                JSONObject obj = new JSONObject(response.getContent());

                Lobby lobby = new Lobby(
                    obj.getString("id"),
                    obj.getInt("utentiConnessi"),
                    obj.getString("rotation"),
                    getUsernameById(obj.getString("creator")),
                    obj.getString("status")
                );

                ArrayList<User> users = new ArrayList<>();
                users.add(LoggedUser.getInstance());
                CurrentLobby.getInstance(lobby, users, new ArrayList<User>());


            } else {
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
            }
        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

    }

    @FXML
    public void initialize() {
        rotationBox.getItems().addAll("Orario", "Antiorario");
        rotationBox.setValue("Orario");
    }

}