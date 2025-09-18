package detalab;

import java.util.ResourceBundle;
import java.net.URL;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import detalab.DTO.LoggedUser;
import detalab.DTO.Response;
import detalab.DTO.Lobby;
import org.json.*;

import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.TableView;
import javafx.scene.control.TableColumn;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.Alert;



public class MainPageController extends GeneralPageController {

    @FXML
    Label welcomeLabel;

    @FXML
    private TableView<Lobby> lobbyList;

    @FXML
    private TableColumn<Lobby, Integer> lobbyID;

    @FXML
    private TableColumn<Lobby, Integer> lobbyUsers;

    @FXML
    private TableColumn<Lobby, String> lobbyRotation;

    @FXML
    private TableColumn<Lobby, String> lobbyCreator;

    @FXML
    private TableColumn<Lobby, String> lobbyStatus;

    @FXML
    private void loadLobbies() {

        try {

            Response response = makeRequest("lobby", "GET", 200);

            String content = response.getContent();

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
            System.out.println("content: " + response.getContent() + "\n");

            if (response.getStatus() == 200) {
                List<Lobby> lobbies = parseLobbies(content);
                lobbyList.getItems().clear();
                lobbyList.getItems().addAll(lobbies);
            } else {
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
            }

            
        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

    }

    private List<Lobby> parseLobbies(String content) {
        
        List<Lobby> lobbies = new ArrayList<>();

        JSONArray array = new JSONArray(content);

        for (int i = 0; i < array.length(); i++) {
            JSONObject obj = array.getJSONObject(i);

            int id = Integer.parseInt(obj.getString("id").trim());
            int users = obj.getInt("utentiConnessi");
            String rotation = obj.getString("rotation");
            String status = obj.getString("status");
            String creator = getUsernameById(obj.getString("creator").trim());

            Lobby lobby = new Lobby(id, users, rotation, creator, status);
            lobbies.add(lobby);
        }

        return lobbies;
    }

    private String getUsernameById(String userID) {
        
        String ret = "Sconosciuto";

        try {

            Response response = makeRequest("user/" + userID, "GET", 200);

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
            System.out.println("content: " + response.getContent() + "\n");

            if (response.getStatus() == 200) {
                String content = response.getContent();
                JSONObject obj = new JSONObject(content);
                ret = obj.getString("nickname").trim();
            }

        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

        return ret;
    }

    // @FXML
    // private void showNewLobbyDialog() throws IOException {
    //     showDialog("NewLobbyDialog", 327, 150, "Creazione Lobby", "main");
    // }

    @FXML
    private void test() {
        System.out.println("Test button clicked");
    }

    @FXML
    private void exit() throws IOException {
        LoggedUser.cleanUserSession();
        App.setRoot("login");
    }

    @FXML
    private void enterGame() throws IOException {
        App.setRoot("game");
    }

    private void setWelcomeMessage() {
        try {
            welcomeLabel.setText("Benvenuto, " + LoggedUser.getInstance().getUsername() + "!");
        } catch (Exception e) {
            e.printStackTrace();
            welcomeLabel.setText("Benvenuto, utente!");
        }
    }

    @FXML
    public void initialize() {

        setWelcomeMessage();

        //Initalize table columns
        lobbyUsers.setCellValueFactory(new PropertyValueFactory<Lobby, Integer>("lobbyUsers"));
        lobbyID.setCellValueFactory(new PropertyValueFactory<Lobby, Integer>("lobbyID"));
        lobbyRotation.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyRotation"));
        lobbyCreator.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyCreator"));
        lobbyStatus.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyStatus"));

        loadLobbies();
    }
}