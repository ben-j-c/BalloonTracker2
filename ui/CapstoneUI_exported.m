classdef CapstoneUI_exported < matlab.apps.AppBase

    % Properties that correspond to app components
    properties (Access = public)
        UIFigure           matlab.ui.Figure
        FileMenu           matlab.ui.container.Menu
        NewMenu            matlab.ui.container.Menu
        OpenMenu           matlab.ui.container.Menu
        SaveAsMenu         matlab.ui.container.Menu
        ExportGraphsMenu   matlab.ui.container.Menu
        WindDirectionMenu  matlab.ui.container.Menu
        WindSpeedMenu      matlab.ui.container.Menu
        PreferencesMenu    matlab.ui.container.Menu
        PlayButton         matlab.ui.control.Button
        StopButton         matlab.ui.control.Button
        TabGroup           matlab.ui.container.TabGroup
        BothTab_2          matlab.ui.container.Tab
        UIAxes             matlab.ui.control.UIAxes
        UIAxes_2           matlab.ui.control.UIAxes
        UITable            matlab.ui.control.Table
        GraphsTab          matlab.ui.container.Tab
        UIAxes_3           matlab.ui.control.UIAxes
        UIAxes_4           matlab.ui.control.UIAxes
        Table              matlab.ui.container.Tab
        UITable_2          matlab.ui.control.Table
    end

    
    methods (Access = private)
        
        function clear_update_graphs(app)
            scatter(app.UIAxes, app.UITable.Data{:,3}, app.UITable.Data{:,2},'filled');
            scatter(app.UIAxes_3, app.UITable.Data{:,3}, app.UITable.Data{:,2},'filled');
            scatter(app.UIAxes_2, app.UITable.Data{:,4}, app.UITable.Data{:,2},'filled');
            scatter(app.UIAxes_4, app.UITable.Data{:,4}, app.UITable.Data{:,2},'filled');
        end
    end
    

    % Callbacks that handle component events
    methods (Access = private)

        % Code that executes after component creation
        function startupFcn(app)
            T = table(0,0,0,0,'VariableNames', {'Time', 'Altitude', 'Direction', 'Speed'});
            app.UITable.Data = T;
            app.UITable_2.Data = app.UITable.Data;
            app.UIFigure.Name = "";
        end

        % Menu selected function: NewMenu
        function NewMenuSelected(app, event)

            msg = 'This will clear any unsaved data.';
            title = 'Confirm New';
            selection = uiconfirm(app.UIFigure,msg,title,...
           'Options',{'Ok','Cancel'},...
           'DefaultOption',2,'CancelOption',2, 'Icon','warning');
            if strcmp(selection, 'Ok')
                T = table(0,0,0,0);
                app.UITable.Data = T;
                app.UITable_2.Data = T;
                cla(app.UIAxes)
                cla(app.UIAxes_2)
                cla(app.UIAxes_3)
                cla(app.UIAxes_4)
                app.UIFigure.Name = "";
            end
        end

        % Menu selected function: OpenMenu
        function OpenMenuSelected(app, event)
            [file, path] = uigetfile('*.csv');
            if (file ~= 0)
                try
                    values = readtable(fullfile(path,file));
                    app.UITable.Data = values(:,{'Time', 'Altitude', 'Direction', 'Speed'});
                    app.UITable_2.Data = app.UITable.Data;
                    clear_update_graphs(app);
                    app.UIFigure.Name = file;
                catch
                    T = table(0,0,0,0);
                    app.UITable.Data = T;
                    app.UITable_2.Data = T;
                    cla(app.UIAxes)
                    cla(app.UIAxes_2)
                    cla(app.UIAxes_3)
                    cla(app.UIAxes_4)
                    app.UIFigure.Name = "";
                    uialert(app.UIFigure,'Operation failed. File does not conform to .csv requirements','Invalid File');
                end
            end
        end

        % Menu selected function: SaveAsMenu
        function SaveAsMenuSelected(app, event)
            [file, path] = uiputfile('*.csv');
            if (file ~= 0)
                writetable(app.UITable.Data, fullfile(path,file), 'WriteVariableNames', true);
                app.UIFigure.Name = file;
            end
        end

        % Menu selected function: WindDirectionMenu
        function WindDirectionMenuSelected(app, event)
            [file, path] = uiputfile('*.png');
            if (file ~= 0)
                temp = figure('Visible','off');
                scatter(app.UITable.Data{:,3}, app.UITable.Data{:,2},'filled');
                temp.CurrentAxes.Title.String = app.UIAxes.Title.String;
                temp.CurrentAxes.XLabel.String = app.UIAxes.XLabel.String;
                temp.CurrentAxes.YLabel.String = app.UIAxes.YLabel.String;
                saveas(temp, fullfile(path, file), 'png');
                delete(temp);
            end            
        end

        % Menu selected function: WindSpeedMenu
        function WindSpeedMenuSelected(app, event)
            [file, path] = uiputfile('*.png');
            if (file ~= 0)
                temp = figure('Visible','off');
                scatter(app.UITable.Data{:,4}, app.UITable.Data{:,2},'filled');
                temp.CurrentAxes.Title.String = app.UIAxes_2.Title.String;
                temp.CurrentAxes.XLabel.String = app.UIAxes_2.XLabel.String;
                temp.CurrentAxes.YLabel.String = app.UIAxes_2.YLabel.String;
                saveas(temp, fullfile(path, file), 'png');
                delete(temp);
            end              
        end

        % Button pushed function: PlayButton
        function PlayButtonPushed(app, event)
            prompt = {'Enter buoyancy of balloon:','Enter timer countdown:'};
            dlgtitle = 'Input';
            dims = [1 35];
            definput = {'5','5'};
            try
                answer = inputdlg(prompt,dlgtitle,dims,definput);
                
                if ~ isempty(answer)
                     app.UIFigure.Name = "Session " + datestr(datetime('now')) + ".csv";
                     app.FileMenu.Enable = 'off';
                     app.PlayButton.Enable = 'off';
                     app.StopButton.Enable = 'on';
                end
            catch
            end
        end

        % Button pushed function: StopButton
        function StopButtonPushed(app, event)
             app.FileMenu.Enable = 'on';
             app.PlayButton.Enable = 'on';
             app.StopButton.Enable = 'off';
        end

        % Display data changed function: UITable
        function UITableDisplayDataChanged(app, event)
            app.UITable_2.Data = app.UITable.Data;
            clear_update_graphs(app)
        end

        % Display data changed function: UITable_2
        function UITable_2DisplayDataChanged(app, event)
            app.UITable.Data = app.UITable_2.Data;
            clear_update_graphs(app)
        end

        % Close request function: UIFigure
        function UIFigureCloseRequest(app, event)
            if (strcmp(app.StopButton.Enable, 'on'))
                uialert(app.UIFigure,'Stop wind tracking session before exiting.','Ongoing Session');
            else
                msg = 'This will clear any unsaved data.';
                title = 'Confirm Exit';
                selection = uiconfirm(app.UIFigure,msg,title,...
               'Options',{'Ok','Cancel'},...
               'DefaultOption',2,'CancelOption',2, 'Icon','warning');
                if strcmp(selection, 'Ok')
                    delete(app);
                end
            end
        end
    end

    % Component initialization
    methods (Access = private)

        % Create UIFigure and components
        function createComponents(app)

            % Create UIFigure and hide until all components are created
            app.UIFigure = uifigure('Visible', 'off');
            app.UIFigure.Position = [100 100 859 447];
            app.UIFigure.Name = 'UI Figure';
            app.UIFigure.CloseRequestFcn = createCallbackFcn(app, @UIFigureCloseRequest, true);

            % Create FileMenu
            app.FileMenu = uimenu(app.UIFigure);
            app.FileMenu.Text = 'File';

            % Create NewMenu
            app.NewMenu = uimenu(app.FileMenu);
            app.NewMenu.MenuSelectedFcn = createCallbackFcn(app, @NewMenuSelected, true);
            app.NewMenu.Text = 'New';

            % Create OpenMenu
            app.OpenMenu = uimenu(app.FileMenu);
            app.OpenMenu.MenuSelectedFcn = createCallbackFcn(app, @OpenMenuSelected, true);
            app.OpenMenu.Text = 'Open';

            % Create SaveAsMenu
            app.SaveAsMenu = uimenu(app.FileMenu);
            app.SaveAsMenu.MenuSelectedFcn = createCallbackFcn(app, @SaveAsMenuSelected, true);
            app.SaveAsMenu.Text = 'Save As';

            % Create ExportGraphsMenu
            app.ExportGraphsMenu = uimenu(app.FileMenu);
            app.ExportGraphsMenu.Text = 'Export Graphs';

            % Create WindDirectionMenu
            app.WindDirectionMenu = uimenu(app.ExportGraphsMenu);
            app.WindDirectionMenu.MenuSelectedFcn = createCallbackFcn(app, @WindDirectionMenuSelected, true);
            app.WindDirectionMenu.Text = 'Wind Direction';

            % Create WindSpeedMenu
            app.WindSpeedMenu = uimenu(app.ExportGraphsMenu);
            app.WindSpeedMenu.MenuSelectedFcn = createCallbackFcn(app, @WindSpeedMenuSelected, true);
            app.WindSpeedMenu.Text = 'Wind Speed';

            % Create PreferencesMenu
            app.PreferencesMenu = uimenu(app.FileMenu);
            app.PreferencesMenu.Text = 'Preferences';

            % Create PlayButton
            app.PlayButton = uibutton(app.UIFigure, 'push');
            app.PlayButton.ButtonPushedFcn = createCallbackFcn(app, @PlayButtonPushed, true);
            app.PlayButton.Icon = 'play.png';
            app.PlayButton.Position = [1 426 65 22];
            app.PlayButton.Text = 'Play';

            % Create StopButton
            app.StopButton = uibutton(app.UIFigure, 'push');
            app.StopButton.ButtonPushedFcn = createCallbackFcn(app, @StopButtonPushed, true);
            app.StopButton.Icon = 'stop.png';
            app.StopButton.Enable = 'off';
            app.StopButton.Position = [65 426 68 22];
            app.StopButton.Text = 'Stop';

            % Create TabGroup
            app.TabGroup = uitabgroup(app.UIFigure);
            app.TabGroup.TabLocation = 'bottom';
            app.TabGroup.Position = [1 19 860 408];

            % Create BothTab_2
            app.BothTab_2 = uitab(app.TabGroup);
            app.BothTab_2.Title = 'Both';

            % Create UIAxes
            app.UIAxes = uiaxes(app.BothTab_2);
            title(app.UIAxes, 'Altitude Vs. Wind Direction')
            xlabel(app.UIAxes, 'X')
            ylabel(app.UIAxes, 'Y')
            app.UIAxes.Position = [11 0 260 368];

            % Create UIAxes_2
            app.UIAxes_2 = uiaxes(app.BothTab_2);
            title(app.UIAxes_2, 'Altitude Vs. Wind Speed')
            xlabel(app.UIAxes_2, 'X')
            ylabel(app.UIAxes_2, 'Y')
            app.UIAxes_2.Position = [270 0 260 368];

            % Create UITable
            app.UITable = uitable(app.BothTab_2);
            app.UITable.ColumnName = {'Time'; 'Altitude'; 'Direction'; 'Speed'};
            app.UITable.RowName = {};
            app.UITable.DisplayDataChangedFcn = createCallbackFcn(app, @UITableDisplayDataChanged, true);
            app.UITable.Position = [537 38 304 331];

            % Create GraphsTab
            app.GraphsTab = uitab(app.TabGroup);
            app.GraphsTab.Title = 'Graphs';

            % Create UIAxes_3
            app.UIAxes_3 = uiaxes(app.GraphsTab);
            title(app.UIAxes_3, 'Altitude Vs. Wind Direction')
            xlabel(app.UIAxes_3, 'X')
            ylabel(app.UIAxes_3, 'Y')
            app.UIAxes_3.Position = [11 5 372 373];

            % Create UIAxes_4
            app.UIAxes_4 = uiaxes(app.GraphsTab);
            title(app.UIAxes_4, 'Altitude Vs. Wind Speed')
            xlabel(app.UIAxes_4, 'X')
            ylabel(app.UIAxes_4, 'Y')
            app.UIAxes_4.Position = [461 5 372 373];

            % Create Table
            app.Table = uitab(app.TabGroup);
            app.Table.Title = 'Table';

            % Create UITable_2
            app.UITable_2 = uitable(app.Table);
            app.UITable_2.ColumnName = {'Time'; 'Altitude'; 'Direction'; 'Speed'};
            app.UITable_2.RowName = {};
            app.UITable_2.DisplayDataChangedFcn = createCallbackFcn(app, @UITable_2DisplayDataChanged, true);
            app.UITable_2.Position = [26 0 808 381];

            % Show the figure after all components are created
            app.UIFigure.Visible = 'on';
        end
    end

    % App creation and deletion
    methods (Access = public)

        % Construct app
        function app = CapstoneUI_exported

            % Create UIFigure and components
            createComponents(app)

            % Register the app with App Designer
            registerApp(app, app.UIFigure)

            % Execute the startup function
            runStartupFcn(app, @startupFcn)

            if nargout == 0
                clear app
            end
        end

        % Code that executes before app deletion
        function delete(app)

            % Delete UIFigure when app is deleted
            delete(app.UIFigure)
        end
    end
end
