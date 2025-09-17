module detalab {
    requires javafx.controls;
    requires javafx.fxml;
    requires org.json;

    opens detalab to javafx.fxml;
    exports detalab;
}
