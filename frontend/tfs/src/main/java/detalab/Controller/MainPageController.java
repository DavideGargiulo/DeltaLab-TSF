package detalab.Controller;

import java.io.IOException;
import java.util.List;
import java.util.Random;
import java.util.ArrayList;
import detalab.DTO.LoggedUser;
import detalab.DTO.Response;
import detalab.DTO.User;
import detalab.DTO.CurrentLobby;
import detalab.DTO.Lobby;
import detalab.DTO.LanguageHelper;
import detalab.App;
import org.json.*;

import javafx.application.Platform;
import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.Button;
import javafx.scene.control.TableView;
import javafx.scene.control.TextField;
import javafx.scene.control.TableColumn;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.control.Alert.AlertType;
import javafx.fxml.FXMLLoader;
import javafx.scene.Node;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.stage.Modality;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;

public class MainPageController extends GeneralPageController {

  @FXML
  private BorderPane mainPage;

  @FXML
  private TextField codeField;

  @FXML
  Label welcomeLabel;

  @FXML
  private TableView<Lobby> lobbyList;

  @FXML
  private TableColumn<Lobby, String> lobbyID;

  @FXML
  private TableColumn<Lobby, Integer> lobbyUsers;

  @FXML
  private TableColumn<Lobby, String> lobbyRotation;

  @FXML
  private TableColumn<Lobby, String> lobbyCreator;

  @FXML
  private TableColumn<Lobby, String> lobbyStatus;

  @FXML
  private Button createLobbyButton;

  @FXML
  private Button randomJoinButton;

  @FXML
  private Button joinButton;

  @FXML
  private Button exitButton;

  @FXML
  private Button updateButton;

  List<String> translatedStrings = new ArrayList<>();

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
        if (lobbies.isEmpty()) {
          installStripedPlaceholder(lobbyList, 30);
        } else {
          lobbyList.getItems().addAll(lobbies);
        }
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

      String id = obj.getString("id");
      int users = obj.getInt("utentiConnessi");
      String rotation = obj.getString("rotation");
      String status = obj.getString("status");
      String creator = getUsernameById(obj.getString("creator").trim());

      Lobby lobby = new Lobby(id, users, rotation, creator, status);
      lobbies.add(lobby);
    }

    return lobbies;
  }

  @FXML
  private void showNewLobbyDialog() {
    try {
      FXMLLoader fxmlLoader = new FXMLLoader(App.class.getResource("create_lobby.fxml"));
      Parent dialogRoot = fxmlLoader.load();
      Scene dialogScene = new Scene(dialogRoot, 365, 180);
      Stage dialogStage = new Stage();
      if (!translatedStrings.isEmpty()) {
        // Translate dialog title
        dialogStage.setTitle(translatedStrings.get(1).split(",")[0].trim());
      } else {
        dialogStage.setTitle("Create Lobby");
      }
      dialogStage.setScene(dialogScene);
      Stage ownerStage = (Stage) mainPage.getScene().getWindow();
      dialogStage.initOwner(ownerStage);
      dialogStage.initModality(Modality.WINDOW_MODAL);
      dialogStage.setResizable(false);
      dialogStage.showAndWait();

      // Things to do after the dialog is closed
      loadLobbies();

      if (CurrentLobby.getInstance() != null) {
        enterLobby(CurrentLobby.getInstance());
      }

    } catch (IOException e) {
      e.printStackTrace();
      showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Non è stato possibile aprire la finestra.");
    }
  }

  @FXML
  private void exit() throws IOException {
    LoggedUser.cleanUserSession();
    Scene scene = mainPage.getScene();
    Stage stage = (Stage) scene.getWindow();
    // Imposta le dimensioni della Scene
    scene.getWindow().setWidth(800 + (stage.getWidth() - scene.getWidth()));
    scene.getWindow().setHeight(500 + (stage.getHeight() - scene.getHeight()));
    stage.centerOnScreen();
    App.setRoot("login");
  }

  @FXML
  private void joinRandomLobby() {
    if (lobbyList.getItems().isEmpty()) {
      showAlert(AlertType.INFORMATION, "Nessuna lobby disponibile", null,
          "Non ci sono lobby disponibili a cui unirsi.");
      return;
    }

    ArrayList<Lobby> randomLobby = new ArrayList<>();
    randomLobby.addAll(lobbyList.getItems());

    Random rand = new Random();

    enterLobby(randomLobby.get(rand.nextInt(randomLobby.size())));
  }

  @FXML
  private void joinLobbyWithID() {
    String lobbyID = codeField.getText();
    if (lobbyID.isEmpty()) {
      showAlert(AlertType.INFORMATION, "Codice mancante", null, "Inserisci un codice per unirti a una lobby.");
      return;
    }
    joinLobbyWithID(lobbyID);
  }

  private void joinLobbyWithID(String lobbyID) {

    try {

      Response response = makeRequest("lobby/" + lobbyID, "GET", 200);

      String content = response.getContent();

      System.out.println("result: " + response.getResult());
      System.out.println("status: " + response.getStatus());
      System.out.println("message: " + response.getMessage());
      System.out.println("content: " + response.getContent() + "\n");

      if (response.getStatus() == 200) {
        JSONObject obj = new JSONObject(content);

        String id = obj.getString("id");
        int users = obj.getInt("utentiConnessi");
        String rotation = obj.getString("rotation");
        String status = obj.getString("status");
        String creator = getUsernameById(obj.getString("creator").trim());

        Lobby lobby = new Lobby(id, users, rotation, creator, status);
        enterLobby(lobby);
      } else {
        showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
      }

    } catch (Exception e) {
      e.printStackTrace();
      showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
    }

    // richiedi lobby con ID = lobbyID
  }

  private void enterLobby(Lobby lobby) {

    try {

      Response response = makeRequest("lobby/" + lobby.getLobbyID() + "/players", "GET", 200);

      System.out.println("result: " + response.getResult());
      System.out.println("status: " + response.getStatus());
      System.out.println("message: " + response.getMessage());
      System.out.println("content: " + response.getContent() + "\n");

      ArrayList<User> players = new ArrayList<>();
      ArrayList<User> spectators = new ArrayList<>();

      JSONObject content = new JSONObject(response.getContent());

      JSONArray activePlayers = content.getJSONArray("activePlayers");
      for (int i = 0; i < activePlayers.length(); i++) {
        JSONObject playerObj = activePlayers.getJSONObject(i);

        User user = new User(playerObj.getInt("id"), playerObj.getString("nickname"), playerObj.getString("status"));

        players.add(user);
      }

      // Estrazione array waitingPlayers
      JSONArray waitingPlayers = content.getJSONArray("waitingPlayers");
      for (int i = 0; i < waitingPlayers.length(); i++) {
        JSONObject playerObj = waitingPlayers.getJSONObject(i);

        User user = new User(playerObj.getInt("id"), playerObj.getString("nickname"), playerObj.getString("status"));

        spectators.add(user);
      }

      CurrentLobby.getInstance(lobby, players, spectators);

      // System.out.println("\n\n----------\n\n");
      // System.out.println(CurrentLobby.getInstance());
      // System.out.println("\n\n----------\n\n");

      App.setRoot("game_screen");
    } catch (Exception e) {
      e.printStackTrace();
      showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
    }

  }

  private void translateUI() {

    try {

      String translatedText = LanguageHelper.translateToNative(
          "Welcome. Create Lobby, Franwik!. Quick Match. Join Lobby. Logout. Update the table, Franwik!. Code. Connected Users. Rotation. Creator. Status. Lobby Code.");

      for (String str : translatedText.split("\\.")) {
        translatedStrings.add(str.trim());
      }

      // Translate welcome message
      welcomeLabel.setText(translatedStrings.get(0) + ",\n" + LoggedUser.getInstance().getUsername() + "!");

      // Translate buttons
      createLobbyButton.setText(translatedStrings.get(1).split(",")[0].trim());
      randomJoinButton.setText(translatedStrings.get(2));
      joinButton.setText(translatedStrings.get(3));
      exitButton.setText(translatedStrings.get(4));
      updateButton.setText(translatedStrings.get(5).split("\\s+")[0]);

      // Translate table columns
      lobbyID.setText(translatedStrings.get(6));
      lobbyUsers.setText(translatedStrings.get(7));
      lobbyRotation.setText(translatedStrings.get(8));
      lobbyCreator.setText(translatedStrings.get(9));
      lobbyStatus.setText(translatedStrings.get(10));

      // Translate Field
      codeField.setPromptText(translatedStrings.get(11));

    } catch (Exception e) {

      // Default translation in case of error

      // Welcome message
      welcomeLabel.setText("Welcome,\nuser!");

      // Buttons
      createLobbyButton.setText("Create Lobby");
      randomJoinButton.setText("Quick Match");
      joinButton.setText("Join");
      exitButton.setText("Log out");
      updateButton.setText("Update");

      // Table columns
      lobbyID.setText("Code");
      lobbyUsers.setText("Connected Users");
      lobbyRotation.setText("Rotation");
      lobbyCreator.setText("Creator");
      lobbyStatus.setText("Status");

      e.printStackTrace();
      showAlert(AlertType.ERROR, "Error", "An error occurred.", "Unable to translate!");
    }
  }

  private void installStripedPlaceholder(TableView<?> table, double rowHeight) {
    table.setFixedCellSize(rowHeight);

    Runnable rebuild = () -> {
      Node header = table.lookup(".column-header-background");
      double headerHeight = header == null ? 0 : header.prefHeight(-1);
      double avail = table.getHeight() - headerHeight;
      int rows = Math.max(1, (int) Math.ceil(avail / rowHeight));

      VBox placeholder = new VBox();
      placeholder.setFillWidth(true);

      for (int r = 0; r < rows; r++) {
        HBox row = new HBox();
        row.setPrefHeight(rowHeight);
        for (TableColumn<?, ?> col : table.getColumns()) {
          Region cell = new Region();
          cell.prefHeightProperty().bind(row.heightProperty());
          // bindiamo la larghezza della "cella finta" alla colonna reale
          cell.prefWidthProperty().bind(col.widthProperty());
          cell.getStyleClass().add(r % 2 == 0 ? "placeholder-cell-even" : "placeholder-cell-odd");
          row.getChildren().add(cell);
        }
        placeholder.getChildren().add(row);
      }

      placeholder.getStyleClass().add("table-placeholder");
      table.setPlaceholder(placeholder);
    };

    // ricostruisci quando la tabella cambia altezza o cambiano le colonne
    table.heightProperty().addListener((o, oldV, newV) -> rebuild.run());
    table.getColumns().forEach(c -> c.widthProperty().addListener((o, oldV, newV) -> rebuild.run()));

    // esegui al next pulse per avere dimensioni corrette
    Platform.runLater(rebuild);
  }

  @FXML
  public void initialize() {

    // Initalize table columns
    lobbyUsers.setCellValueFactory(new PropertyValueFactory<Lobby, Integer>("lobbyUsers"));
    lobbyID.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyID"));
    lobbyRotation.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyRotation"));
    lobbyCreator.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyCreator"));
    lobbyStatus.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyStatus"));

    // Set row factory for lobbyList
    lobbyList.setRowFactory(tv -> {
      javafx.scene.control.TableRow<Lobby> row = new javafx.scene.control.TableRow<>();
      row.setOnMouseClicked(event -> {
        if (event.getClickCount() == 2 && (!row.isEmpty())) {
          Lobby lobby = row.getItem();
          System.out.println(lobby.toString());
          joinLobbyWithID(lobby.getLobbyID());
        }
      });
      return row;
    });

    loadLobbies();

    translateUI();
  }
}