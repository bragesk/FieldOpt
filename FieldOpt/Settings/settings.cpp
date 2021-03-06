/******************************************************************************
   Copyright (C) 2015-2017 Einar J.M. Baumann <einar.baumann@gmail.com>

   This file is part of the FieldOpt project.

   FieldOpt is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   FieldOpt is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with FieldOpt.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "settings.h"
#include "settings_exceptions.h"
#include "Utilities/filehandling.hpp"

#include <QJsonDocument>
#include <iostream>


namespace Settings {

    Settings::Settings(Paths &paths)
    {
        paths_ = paths;
        readDriverFile();
    }

    QString Settings::GetLogCsvString() const
    {
        QStringList header  = QStringList();
        QStringList content = QStringList();
        header  << "name"
                << "maxevals"
                << "initstep"
                << "minstep";
        content << name_
                << QString::number(optimizer_->parameters().max_evaluations)
                << QString::number(optimizer_->parameters().initial_step_length)
                << QString::number(optimizer_->parameters().minimum_step_length);

        return QString("%1\n%2").arg(header.join(",")).arg(content.join(","));
    }

    void Settings::readDriverFile()
    {
        QFile *file = new QFile(QString::fromStdString(paths_.GetPath(Paths::DRIVER_FILE)));
        if (!file->open(QIODevice::ReadOnly))
            throw DriverFileReadException("Unable to open the driver file");

        QByteArray data = file->readAll();

        QJsonDocument json = QJsonDocument::fromJson(data);
        if (json.isNull())
            throw DriverFileJsonParsingException("Unable to parse the input file to JSON.");

        if (!json.isObject())
            throw DriverFileFormatException("Driver file format incorrect. Must be a JSON object.");

        json_driver_ = new QJsonObject(json.object());

        readGlobalSection();
        readSimulatorSection();
        readModelSection();
        readOptimizerSection();

        file->close();
    }

    void Settings::readGlobalSection()
    {
        try {
            QJsonObject global = json_driver_->value("Global").toObject();
            name_ = global["Name"].toString();
            bookkeeper_tolerance_ = global["BookkeeperTolerance"].toDouble();
            if (bookkeeper_tolerance_ < 0.0) throw UnableToParseGlobalSectionException("The bookkeeper tolerance must be a positive number.");
        }
        catch (std::exception const &ex) {
            throw UnableToParseGlobalSectionException("Unable to parse driver file global section: " + std::string(ex.what()));
        }
    }

    void Settings::readSimulatorSection()
    {
        // Simulator root
        try {
            QJsonObject json_simulator = json_driver_->value("Simulator").toObject();
            simulator_ = new Simulator(json_simulator, paths_);
        }
        catch (std::exception const &ex) {
            throw UnableToParseSimulatorSectionException("Unable to parse driver file simulator section: " + std::string(ex.what()));
        }
    }

    void Settings::readOptimizerSection()
    {
        try {
            QJsonObject optimizer = json_driver_->value("Optimizer").toObject();
            optimizer_ = new Optimizer(optimizer);
        }
        catch (std::exception const &ex) {
            throw UnableToParseOptimizerSectionException("Unable to parse driver file optimizer section: " + std::string(ex.what()));
        }
    }

    void Settings::readModelSection()
    {
        try {
            QJsonObject model = json_driver_->value("Model").toObject();
            model_ = new Model(model, paths_);
        }
        catch (std::exception const &ex) {
            throw UnableToParseModelSectionException("Unable to parse model section: " + std::string(ex.what()));
        }
    }

}

