module detalab {
    requires javafx.controls;
    requires javafx.fxml;

    opens detalab to javafx.fxml;
    exports detalab;
}
